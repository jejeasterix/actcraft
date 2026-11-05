// Programme d'essai des raccourcis (L6-T29) : écrit le fichier dont le chemin est son premier argument, puis
// sort. Le chemin arrive entier (espaces, « & ») seulement si aucun interpréteur ne découpe la ligne.
// std::ofstream plutôt que fopen : MSVC classe fopen « unsafe » (C4996), erreur sous /W4 /WX.
#include <fstream>

int main(int argc, char** argv)
{
    if (argc < 2)
        return 2;
    std::ofstream f(argv[1], std::ios::binary);
    if (!f)
        return 1;
    f << "lance";
    return 0;
}
