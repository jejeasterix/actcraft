#pragma once

namespace actcraft::ui {

// À appeler avant QGuiApplication, dans chaque main : PC et Mac, partage des contextes et limite des
// processus de rendu (seule une importation QML différée charge le moteur, DA2) ; Android,
// QtWebView::initialize().
void prepareWebFrameEngine();

} // namespace actcraft::ui
