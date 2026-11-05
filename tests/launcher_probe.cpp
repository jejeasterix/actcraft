// Programme d'essai des raccourcis (L6-T29) : écrit le fichier dont le chemin est son premier argument, puis
// sort. Le chemin arrive entier (espaces, « & ») seulement si aucun interpréteur ne découpe la ligne.
#include <cstdio>

int main(int argc, char** argv)
{
    if (argc < 2)
        return 2;
    std::FILE* f = std::fopen(argv[1], "wb");
    if (!f)
        return 1;
    std::fputs("lance", f);
    std::fclose(f);
    return 0;
}
