# Bibliothèque Numérique — Système de gestion en C

Projet de gestion d'une bibliothèque numérique développé en langage C.  
Le programme tourne en ligne de commande et gère les livres, les lecteurs, les emprunts et les retours.

---

## Fonctionnalités

### Mode Admin (mot de passe : `admin123`)
- Ajouter un livre au catalogue (ou augmenter le stock s'il existe déjà)
- Afficher le catalogue complet avec disponibilité
- Voir les livres récemment ajoutés
- Voir la file d'attente des lecteurs en attente d'un livre
- Consulter l'historique de tous les lecteurs

### Mode Lecteur (connexion par ID)
- Emprunter un livre (réduit le stock de 1)
- Retourner un livre (augmente le stock de 1)
- Être mis en file d'attente automatiquement si le stock est à 0
- Recevoir une notification quand le livre devient disponible
- Consulter son historique personnel
- Suivre ses points (bonus/pénalité selon les retards)

---

## Structures de données utilisées

| Structure | Type | Usage |
|-----------|------|-------|
| `livre` | Liste chaînée | Catalogue des livres |
| `utilisateur` | Liste chaînée | Liste des lecteurs inscrits |
| `pile_livre` | Pile (LIFO) | Livres récemment ajoutés |
| `pile_historique` | Pile (LIFO) | Historique des emprunts par lecteur |
| `file_attente` | File (FIFO) | File d'attente pour livres indisponibles |

---

## Fichiers générés à l'exécution

> Ces fichiers sont créés automatiquement au premier lancement.  
> **Ne pas les inclure dans le dépôt GitHub** (voir `.gitignore`).

| Fichier | Contenu |
|---------|---------|
| `bibliotheque.txt` | Catalogue des livres (titre, auteur, catégorie, année, stock) |
| `utilisateurs.txt` | Comptes lecteurs avec points et historique complet |
| `historique.txt` | Log global de toutes les opérations (emprunts / retours) |

---

## Catalogue initial

Au premier lancement, 12 livres sont chargés automatiquement :

- **Romans** : Le Petit Prince, Les Misérables, L'Étranger, Madame Bovary, Germinal  
- **Sci-Fi** : 1984, Le Meilleur des Mondes, Fahrenheit 451  
- **Philosophie** : Le Mythe de Sisyphe, Ainsi Parlait Zarathoustra  
- **Informatique** : Le Langage C, Introduction aux Algorithmes  

---

## Compilation et exécution

### Prérequis
- Compilateur GCC (ou tout compilateur C standard)
- Terminal Linux / macOS / Windows (MinGW)

### Compiler
```bash
gcc -o biblio bibliotheque.c
```

### Lancer
```bash
./biblio
```

### Sur Windows
```bash
gcc -o biblio.exe bibliotheque.c
biblio.exe
```

---

## Utilisation rapide

```
=== BIBLIOTHEQUE NUMERIQUE ===
1. Connexion Admin
2. Connexion Lecteur
3. Quitter
```

**Connexion admin** → entrer le mot de passe : `admin123`

**Connexion lecteur** → entrer un ID (nombre entier).  
Si l'ID n'existe pas encore, le programme crée automatiquement le compte.

---

## Système de points

| Action | Points |
|--------|--------|
| Retour dans les délais | +10 pts |
| Retour en retard | -10 pts |

La durée d'emprunt est de **30 jours**. La date de retour prévue est calculée automatiquement à partir de la date d'emprunt saisie.

---

## Structure du projet

```
bibliotheque-numerique/
│
├── bibliotheque.c       ← code source principal (seul fichier à pousser)
├── README.md            ← ce fichier
└── .gitignore           ← fichiers à exclure du dépôt
```

---

## Auteur

Projet réalisé dans le cadre d'un cours de structures de données en C.
