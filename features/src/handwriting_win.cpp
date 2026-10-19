#include <actcraft/features/handwriting.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <utility>
#include <vector>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
// COM et automatisation avant l'encre : msinkaut.h emploie le mot « interface », défini par objbase.h (que
// windows.h n'inclut pas sous WIN32_LEAN_AND_MEAN).
#include <objbase.h>
#include <oleauto.h>
// Encre de Windows.
#include <msinkaut.h>
#include <wrl/client.h>

namespace actcraft::features {

namespace {

using Microsoft::WRL::ComPtr;

// Unité de l'encre de Windows : le HIMETRIC (1/100 mm). Un pixel de page (1/96 de pouce) en vaut 2540/96.
constexpr double kHimetricPerPixel = 2540.0 / 96.0;
// Alternatives demandées au moteur, en plus du meilleur texte (F-295 : 10 au plus).
constexpr long kMaxAlternates = 10;

QString fromBstr(BSTR text)
{
    return text ? QString::fromWCharArray(text, static_cast<qsizetype>(SysStringLen(text))) : QString();
}

// BSTR rendu par un accesseur COM, libéré à la sortie de portée.
class OwnedBstr
{
public:
    OwnedBstr() = default;
    ~OwnedBstr() { SysFreeString(m_text); }
    OwnedBstr(const OwnedBstr&) = delete;
    OwnedBstr& operator=(const OwnedBstr&) = delete;

    BSTR* out() { return &m_text; }
    QString text() const { return fromBstr(m_text); }

private:
    BSTR m_text = nullptr;
};

// VARIANT libéré à la sortie de portée (VariantClear détruit aussi le SAFEARRAY qu'il porte).
class OwnedVariant
{
public:
    OwnedVariant() { VariantInit(&m_value); }
    ~OwnedVariant() { VariantClear(&m_value); }
    OwnedVariant(const OwnedVariant&) = delete;
    OwnedVariant& operator=(const OwnedVariant&) = delete;

    VARIANT* out() { return &m_value; }
    const VARIANT& value() const { return m_value; }

private:
    VARIANT m_value;
};

LONG toHimetric(double pixels)
{
    return static_cast<LONG>(std::lround(pixels * kHimetricPerPixel));
}

// Langues d'un reconnaisseur ; vide en cas d'erreur ou sans langue (gestes). Le tableau porte des
// identifiants de langue (16 bits) ou des LCID (32 bits) selon le moteur : les deux se lisent.
std::vector<LANGID> languagesOf(IInkRecognizer* recognizer)
{
    std::vector<LANGID> out;
    OwnedVariant languages;
    if (FAILED(recognizer->get_Languages(languages.out())) || (languages.value().vt & VT_ARRAY) == 0 ||
        !languages.value().parray)
        return out;
    SAFEARRAY* array = languages.value().parray;
    const auto readable = [](VARTYPE t) { return t == VT_I2 || t == VT_UI2 || t == VT_I4 || t == VT_UI4; };
    // Type des éléments : celui que porte le VARIANT d'abord (vu sur un poste Windows 11 : VT_ARRAY | VT_I2),
    // celui du tableau ensuite (SafeArrayGetVartype échoue sur un tableau créé sans FADF_HAVEVARTYPE).
    VARTYPE type = static_cast<VARTYPE>(languages.value().vt & VT_TYPEMASK);
    if (!readable(type) && (FAILED(SafeArrayGetVartype(array, &type)) || !readable(type)))
        return out;
    // SafeArrayGetElement copie la taille d'élément du tableau : elle doit être celle du type annoncé, sinon
    // la lecture déborderait la variable de destination.
    const bool lcids = type == VT_I4 || type == VT_UI4;
    const std::size_t expectedSize = lcids ? sizeof(LONG) : sizeof(SHORT);
    if (static_cast<std::size_t>(SafeArrayGetElemsize(array)) != expectedSize)
        return out;
    LONG lower = 0;
    LONG upper = -1;
    if (FAILED(SafeArrayGetLBound(array, 1, &lower)) || FAILED(SafeArrayGetUBound(array, 1, &upper)))
        return out;
    for (LONG i = lower; i <= upper; ++i) {
        if (lcids) {
            LONG lcid = 0;
            if (SUCCEEDED(SafeArrayGetElement(array, &i, &lcid)))
                out.push_back(LANGIDFROMLCID(static_cast<LCID>(lcid)));
        } else {
            SHORT language = 0;
            if (SUCCEEDED(SafeArrayGetElement(array, &i, &language)))
                out.push_back(static_cast<LANGID>(language));
        }
    }
    return out;
}

bool hasFrench(const std::vector<LANGID>& languages)
{
    return std::any_of(languages.begin(), languages.end(),
                       [](LANGID language) { return PRIMARYLANGID(language) == LANG_FRENCH; });
}

// Ajoute un trait à l'encre : paquets (x0, y0, x1, y1, …) en HIMETRIC, dans un SAFEARRAY de LONG.
bool addStroke(IInkDisp* ink, const std::vector<QPointF>& points)
{
    SAFEARRAY* packets = SafeArrayCreateVector(VT_I4, 0, static_cast<ULONG>(points.size() * 2));
    if (!packets)
        return false;
    OwnedVariant packetData; // propriétaire du SAFEARRAY dès ici
    packetData.out()->vt = static_cast<VARTYPE>(VT_ARRAY | VT_I4);
    packetData.out()->parray = packets;
    LONG* data = nullptr;
    if (FAILED(SafeArrayAccessData(packets, reinterpret_cast<void**>(&data))))
        return false;
    for (std::size_t i = 0; i < points.size(); ++i) {
        data[2 * i] = toHimetric(points[i].x());
        data[2 * i + 1] = toHimetric(points[i].y());
    }
    SafeArrayUnaccessData(packets);
    OwnedVariant layout; // vide : description par défaut des paquets (X, Y)
    ComPtr<IInkStrokeDisp> stroke;
    return SUCCEEDED(ink->CreateStroke(packetData.value(), layout.value(), &stroke));
}

// Texte d'une alternative, lu par IDispatch : propriété DISPID_InkRecoAlternate_String de msinkaut.h.
QString alternateText(IInkRecognitionAlternate* alternate)
{
    DISPPARAMS noArguments{nullptr, nullptr, 0, 0};
    OwnedVariant value;
    if (FAILED(alternate->Invoke(DISPID_InkRecoAlternate_String, IID_NULL, LOCALE_USER_DEFAULT,
                                 DISPATCH_PROPERTYGET, &noArguments, value.out(), nullptr, nullptr)) ||
        value.value().vt != VT_BSTR)
        return {};
    return fromBstr(value.value().bstrVal);
}

class WindowsInkRecognizer final : public HandwritingRecognizer
{
public:
    WindowsInkRecognizer(ComPtr<IInkRecognizer> engine, QString name)
        : m_engine(std::move(engine)), m_name(std::move(name))
    {}

    bool available() const override { return true; }
    QString status() const override { return m_name; }

    QStringList recognize(const InkStrokes& strokes) override
    {
        // Toute erreur COM rend une liste vide : jamais d'exception ni d'arrêt.
        ComPtr<IInkDisp> ink;
        if (FAILED(CoCreateInstance(__uuidof(InkDisp), nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&ink))))
            return {};
        bool any = false;
        for (const std::vector<QPointF>& points : strokes) {
            if (points.empty())
                continue;
            if (!addStroke(ink.Get(), points))
                return {};
            any = true;
        }
        if (!any)
            return {};
        ComPtr<IInkStrokes> inkStrokes;
        ComPtr<IInkRecognizerContext> context;
        if (FAILED(ink->get_Strokes(&inkStrokes)) || FAILED(m_engine->CreateRecognizerContext(&context)) ||
            FAILED(context->putref_Strokes(inkStrokes.Get())))
            return {};
        context->EndInkInput(); // plus aucun trait à venir : résultat définitif (échec sans conséquence)
        InkRecognitionStatus recognition = IRS_NoError;
        ComPtr<IInkRecognitionResult> result;
        if (FAILED(context->Recognize(&recognition, &result)) || !result)
            return {};

        QStringList texts;
        const auto add = [&texts](const QString& text) {
            if (!text.isEmpty() && !texts.contains(text))
                texts.append(text);
        };
        OwnedBstr top;
        if (SUCCEEDED(result->get_TopString(top.out())))
            add(top.text());
        ComPtr<IInkRecognitionAlternates> alternates;
        long count = 0;
        if (FAILED(result->AlternatesFromSelection(0, -1, kMaxAlternates, &alternates)) || !alternates ||
            FAILED(alternates->get_Count(&count)))
            return texts;
        for (long i = 0; i < count; ++i) {
            ComPtr<IInkRecognitionAlternate> alternate;
            if (SUCCEEDED(alternates->Item(i, &alternate)) && alternate)
                add(alternateText(alternate.Get()));
        }
        return texts;
    }

private:
    ComPtr<IInkRecognizer> m_engine;
    QString m_name;
};

} // namespace

std::unique_ptr<HandwritingRecognizer> makePlatformHandwritingRecognizer()
{
    // Sur un fil sans appartement COM, CoCreateInstance échoue (CO_E_NOTINITIALIZED) : nullptr, et l'appelant
    // retombe sur « indisponible ». La reconnaissance se crée sur HandwritingThread, dans son appartement.
    ComPtr<IInkRecognizers> recognizers;
    long count = 0;
    if (FAILED(CoCreateInstance(__uuidof(InkRecognizers), nullptr, CLSCTX_INPROC_SERVER,
                                IID_PPV_ARGS(&recognizers))) ||
        FAILED(recognizers->get_Count(&count)))
        return nullptr;
    // Le premier reconnaisseur qui lit le français, sinon le premier qui a une langue.
    ComPtr<IInkRecognizer> chosen;
    for (long i = 0; i < count; ++i) {
        ComPtr<IInkRecognizer> candidate;
        if (FAILED(recognizers->Item(i, &candidate)) || !candidate)
            continue;
        const std::vector<LANGID> languages = languagesOf(candidate.Get());
        if (languages.empty())
            continue;
        if (hasFrench(languages)) {
            chosen = candidate;
            break;
        }
        if (!chosen)
            chosen = candidate;
    }
    if (!chosen)
        return nullptr;
    OwnedBstr name;
    QString label;
    if (SUCCEEDED(chosen->get_Name(name.out())))
        label = name.text();
    if (label.isEmpty())
        label = QStringLiteral("Moteur d'encre de Windows");
    return std::make_unique<WindowsInkRecognizer>(std::move(chosen), label);
}

bool enterRecognitionApartment()
{
    // S_OK (appartement ouvert) et S_FALSE (déjà ouvert en STA sur ce fil) demandent chacun un CoUninitialize
    // sur ce fil ; RPC_E_CHANGED_MODE (fil déjà en MTA) n'en demande pas.
    return SUCCEEDED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED));
}

void leaveRecognitionApartment()
{
    CoUninitialize();
}

} // namespace actcraft::features
