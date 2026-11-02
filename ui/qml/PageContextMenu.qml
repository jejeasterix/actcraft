import QtQuick
import QtQuick.Controls.Basic
import ActCraft.Ui

// Menu contextuel de la page (F-202, partie) : les entrées de commands.contextMenu, sauf celle de l'outil
// courant (déjà actif : « Stylo » en premier avec Sélectionner, « Sélectionner » avec le stylo ou le marqueur).
// Libellés, raccourcis et état lus des commandes ; un clic déclenche la commande par son nom. Ouvert par
// PageArea à la demande de la vue (contextMenuRequested).
CommandMenu {
    id: ctx
    objectName: "pageContextMenu"
    // Entrées à montrer : l'outil courant (commande « tool.* » cochée) est retiré, puis les séparateurs de tête,
    // de queue et doublés.
    function visibleEntries() {
        const out = []
        const all = ctx.commands.contextMenu
        for (let i = 0; i < all.length; ++i) {
            const item = all[i]
            const separator = item.separator === true
            if (!separator && String(item.id).startsWith("tool.") && ctx.commands.isChecked(item.id))
                continue
            if (separator && (out.length === 0 || out[out.length - 1].separator === true))
                continue
            out.push(item)
        }
        while (out.length > 0 && out[out.length - 1].separator === true)
            out.pop()
        return out
    }
    function popupAt(x, y) {
        ctx.spec = { id: "", label: "", items: ctx.visibleEntries() }
        ctx.popup(x, y)
    }
}