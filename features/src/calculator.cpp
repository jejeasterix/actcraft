#include <actcraft/features/calculator.h>

#include <cmath>

namespace actcraft::features {

namespace {

constexpr int kMaxDigits = 12;

QString operatorSymbol(QChar op)
{
    switch (op.unicode()) {
    case '*':
        return QStringLiteral("×");
    case '/':
        return QStringLiteral("÷");
    case '-':
        return QStringLiteral("−");
    default:
        return QStringLiteral("+");
    }
}

int digitCount(const QString& text)
{
    int n = 0;
    for (const QChar c : text) {
        if (c.isDigit())
            ++n;
    }
    return n;
}

} // namespace

QString Calculator::display() const
{
    return m_display;
}

QString Calculator::expression() const
{
    return m_expression;
}

QString Calculator::pageText() const
{
    return m_done ? m_expression + QLatin1Char(' ') + m_display : m_display;
}

double Calculator::entry() const
{
    QString text = m_display;
    text.replace(QLatin1Char(','), QLatin1Char('.'));
    return text.toDouble();
}

void Calculator::showValue(double value)
{
    if (!std::isfinite(value)) {
        m_display = QStringLiteral("Erreur");
        m_error = true;
        return;
    }
    if (value == 0)
        value = 0; // écarte le zéro négatif
    m_display = QString::number(value, 'g', kMaxDigits);
    m_display.replace(QLatin1Char('.'), QLatin1Char(','));
}

void Calculator::apply()
{
    const double e = entry();
    switch (m_pending.unicode()) {
    case '+':
        m_accumulator += e;
        break;
    case '-':
        m_accumulator -= e;
        break;
    case '*':
        m_accumulator *= e;
        break;
    case '/':
        m_accumulator /= e;
        break;
    default:
        m_accumulator = e;
        break;
    }
    showValue(m_accumulator);
}

void Calculator::press(const QString& key)
{
    if (key == QLatin1String("C")) {
        m_expression.clear();
        m_done = false;
        m_display = QStringLiteral("0");
        m_accumulator = 0;
        m_pending = QChar();
        m_newEntry = true;
        m_recalled = false;
        m_error = false;
        return;
    }
    if (m_error)
        return;

    const auto append = [this](const QString& text) {
        if (!m_expression.isEmpty())
            m_expression += QLatin1Char(' ');
        m_expression += text;
    };

    if (key.size() == 1 && (key[0].isDigit() || key[0] == QLatin1Char(','))) {
        if (m_done) {
            m_expression.clear();
            m_done = false;
        }
        m_recalled = false;
        const bool comma = key[0] == QLatin1Char(',');
        if (m_newEntry) {
            m_display = comma ? QStringLiteral("0,") : key;
            m_newEntry = false;
        } else if (comma) {
            if (!m_display.contains(QLatin1Char(',')))
                m_display += key;
        } else if (digitCount(m_display) < kMaxDigits) {
            if (m_display == QLatin1String("0"))
                m_display = key;
            else if (m_display == QLatin1String("-0"))
                m_display = QLatin1Char('-') + key;
            else
                m_display += key;
        }
    } else if (key == QLatin1String("+") || key == QLatin1String("-") || key == QLatin1String("*") ||
               key == QLatin1String("/")) {
        const QChar op = key[0];
        if (m_done) {
            m_expression.clear();
            m_done = false;
        }
        if (!m_pending.isNull() && m_newEntry && !m_recalled) {
            m_expression.chop(1); // deux opérations de suite : la seconde remplace la première
            m_expression += operatorSymbol(op);
        } else {
            append(m_display + QLatin1Char(' ') + operatorSymbol(op));
            if (m_pending.isNull())
                m_accumulator = entry();
            else
                apply();
        }
        m_pending = op;
        m_newEntry = true;
        m_recalled = false;
    } else if (key == QLatin1String("=")) {
        if (m_done)
            m_expression.clear();
        append(m_display + QStringLiteral(" ="));
        if (!m_pending.isNull())
            apply();
        m_pending = QChar();
        m_done = true;
        m_newEntry = true;
        m_recalled = false;
    } else if (key == QLatin1String("%")) {
        if (m_done)
            m_expression.clear();
        const double e = entry();
        double result = e / 100;
        switch (m_pending.unicode()) {
        case '*':
            result = m_accumulator * e / 100;
            break;
        case '/':
            result = m_accumulator / (e / 100);
            break;
        case '+':
            result = m_accumulator + m_accumulator * e / 100;
            break;
        case '-':
            result = m_accumulator - m_accumulator * e / 100;
            break;
        default:
            break;
        }
        append(m_display + QStringLiteral(" % ="));
        m_accumulator = result;
        showValue(result);
        m_pending = QChar();
        m_done = true;
        m_newEntry = true;
        m_recalled = false;
    } else if (key == QStringLiteral("±")) {
        if (m_display != QLatin1String("0")) {
            if (m_display.startsWith(QLatin1Char('-')))
                m_display.remove(0, 1);
            else
                m_display.prepend(QLatin1Char('-'));
        }
    } else if (key == QLatin1String("CE")) {
        m_display = QStringLiteral("0");
        m_newEntry = true;
        m_recalled = false;
    } else if (key == QLatin1String("M+")) {
        m_memory += entry();
        m_newEntry = true;
    } else if (key == QLatin1String("M-")) {
        m_memory -= entry();
        m_newEntry = true;
    } else if (key == QLatin1String("MR")) {
        showValue(m_memory);
        m_newEntry = true;
        m_recalled = true;
    } else if (key == QLatin1String("MC")) {
        m_memory = 0;
    }
}

} // namespace actcraft::features
