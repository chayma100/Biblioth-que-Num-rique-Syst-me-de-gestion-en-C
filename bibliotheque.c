#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* ============================================================
   STRUCTURES DE DONNÉES
   ============================================================ */

/* --- Livre (liste chaînée) --- */
typedef struct livre {
    char titre[100];
    char auteur[50];
    char categorie[50];
    int  annee;
    int  stock;
    struct livre *suivant;
} livre;

/* --- Noeud pile livres recents (LIFO) --- */
typedef struct pile_livre {
    char titre[100];
    char auteur[50];
    struct pile_livre *suivant;
} pile_livre;

/* --- Date --- */
typedef struct date {
    int jour, mois, annee;
} date;

/* --- Noeud pile historique lus (LIFO) --- */
typedef struct pile_historique {
    char titre[100];
    char auteur[50];
    date date_op;
    char operation[10]; /* "emprunt" ou "retour" */
    struct pile_historique *suivant;
} pile_historique;

/* --- Noeud file d'attente emprunt (FIFO) --- */
typedef struct file_attente {
    int  id_lecteur;
    char nom_lecteur[50];
    char titre[100];
    char auteur[50];
    struct file_attente *suivant;
} file_attente;

/* --- Utilisateur / Lecteur --- */
typedef struct utilisateur {
    int  ID;
    char nom[50];
    int  points;
    pile_historique *histo_tete; /* sommet de la pile historique */
    struct utilisateur *suivant;
} utilisateur;

/* ============================================================
   VARIABLES GLOBALES
   ============================================================ */
livre        *biblio       = NULL;
pile_livre   *pile_recents = NULL;
file_attente *file_tete    = NULL;
file_attente *file_queue   = NULL;
utilisateur  *liste_users  = NULL;

#define MOT_DE_PASSE_ADMIN "admin123"

/* ============================================================
   UTILITAIRES
   ============================================================ */
void vider_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

void afficher_separateur() {
    printf("\n------------------------------------------------\n");
}

date saisir_date() {
    date d;
    printf("  Date (jj/mm/aaaa) : ");
    scanf("%d/%d/%d", &d.jour, &d.mois, &d.annee);
    vider_buffer();
    return d;
}

date calculer_date_retour(date d) {
    d.jour += 30;
    while (1) {
        int nb;
        if (d.mois==1||d.mois==3||d.mois==5||d.mois==7||
            d.mois==8||d.mois==10||d.mois==12) nb=31;
        else if (d.mois==4||d.mois==6||d.mois==9||d.mois==11) nb=30;
        else nb=((d.annee%4==0&&d.annee%100!=0)||(d.annee%400==0))?29:28;
        if (d.jour > nb) { d.jour -= nb; d.mois++; if (d.mois>12){d.mois=1;d.annee++;} }
        else break;
    }
    return d;
}

int date_superieure(date d1, date d2) {
    if (d1.annee != d2.annee) return d1.annee > d2.annee;
    if (d1.mois  != d2.mois)  return d1.mois  > d2.mois;
    return d1.jour > d2.jour;
}

/* ============================================================
   PILE — LIVRES RÉCEMMENT AJOUTÉS
   ============================================================ */
void push_recent(const char *titre, const char *auteur) {
    pile_livre *nv = malloc(sizeof(pile_livre));
    if (!nv) { printf("Erreur memoire\n"); return; }
    strcpy(nv->titre,  titre);
    strcpy(nv->auteur, auteur);
    nv->suivant  = pile_recents;
    pile_recents = nv;
}

void afficher_pile_recents() {
    if (!pile_recents) { printf("  Aucun livre recemment ajoute.\n"); return; }
    printf("  Livres recemment ajoutes (du plus recent au plus ancien) :\n");
    pile_livre *p = pile_recents;
    int i = 1;
    while (p) {
        printf("  %d. \"%s\" — %s\n", i++, p->titre, p->auteur);
        p = p->suivant;
    }
}

/* ============================================================
   PILE — HISTORIQUE DES LIVRES LUS (par lecteur)
   ============================================================ */
void push_historique(utilisateur *u, const char *titre, const char *auteur,
                     const char *op, date d) {
    pile_historique *nv = malloc(sizeof(pile_historique));
    if (!nv) { printf("Erreur memoire\n"); return; }
    strcpy(nv->titre,     titre);
    strcpy(nv->auteur,    auteur);
    strcpy(nv->operation, op);
    nv->date_op   = d;
    nv->suivant   = u->histo_tete;
    u->histo_tete = nv;
}

void afficher_historique_lecteur(utilisateur *u) {
    if (!u->histo_tete) {
        printf("  Historique de %s : vide.\n", u->nom);
        return;
    }
    printf("  Historique de %s (du plus recent au plus ancien) :\n", u->nom);
    pile_historique *p = u->histo_tete;
    int i = 1;
    while (p) {
        printf("  %d. [%-7s] \"%s\" — %s  (%02d/%02d/%d)\n",
               i++, p->operation, p->titre, p->auteur,
               p->date_op.jour, p->date_op.mois, p->date_op.annee);
        p = p->suivant;
    }
}

/* Chercher la date du dernier emprunt d'un livre dans la pile historique */
int chercher_date_emprunt(utilisateur *u, const char *titre,
                          const char *auteur, date *out) {
    pile_historique *p = u->histo_tete;
    while (p) {
        if (strcmp(p->operation, "emprunt") == 0 &&
            strcmp(p->titre,  titre)  == 0 &&
            strcmp(p->auteur, auteur) == 0) {
            *out = p->date_op;
            return 1; /* sommet de pile = emprunt le plus recent */
        }
        p = p->suivant;
    }
    return 0;
}

/* ============================================================
   FILE D'ATTENTE — EMPRUNT (FIFO)
   ============================================================ */
void enqueue(int id, const char *nom, const char *titre, const char *auteur) {
    file_attente *tmp = file_tete;
    while (tmp) {
        if (tmp->id_lecteur == id &&
            strcmp(tmp->titre,  titre)  == 0 &&
            strcmp(tmp->auteur, auteur) == 0) {
            printf("  Vous etes deja en file d'attente pour ce livre.\n");
            return;
        }
        tmp = tmp->suivant;
    }
    file_attente *nv = malloc(sizeof(file_attente));
    if (!nv) { printf("Erreur memoire\n"); return; }
    nv->id_lecteur = id;
    strcpy(nv->nom_lecteur, nom);
    strcpy(nv->titre,  titre);
    strcpy(nv->auteur, auteur);
    nv->suivant = NULL;
    if (!file_queue) { file_tete = file_queue = nv; }
    else             { file_queue->suivant = nv; file_queue = nv; }
    printf("  Livre indisponible. Vous avez ete ajoute(e) a la file d'attente.\n");
}

void notifier_file(const char *titre, const char *auteur) {
    file_attente *prev = NULL, *p = file_tete;
    while (p) {
        if (strcmp(p->titre,  titre)  == 0 &&
            strcmp(p->auteur, auteur) == 0) {
            printf("  >>> Notification : \"%s\" est disponible pour %s (ID %d)\n",
                   titre, p->nom_lecteur, p->id_lecteur);
            if (prev) prev->suivant = p->suivant;
            else      file_tete     = p->suivant;
            if (p == file_queue) file_queue = prev;
            free(p);
            return;
        }
        prev = p; p = p->suivant;
    }
}

void afficher_file_attente() {
    if (!file_tete) { printf("  File d'attente vide.\n"); return; }
    printf("  File d'attente :\n");
    file_attente *p = file_tete;
    int i = 1;
    while (p) {
        printf("  %d. %s (ID %d) attend \"%s\" — %s\n",
               i++, p->nom_lecteur, p->id_lecteur, p->titre, p->auteur);
        p = p->suivant;
    }
}

/* ============================================================
   LISTE CHAÎNÉE — LIVRES
   ============================================================ */
livre *chercher_livre(const char *titre, const char *auteur) {
    livre *p = biblio;
    while (p) {
        if (strcmp(p->titre, titre)==0 && strcmp(p->auteur, auteur)==0) return p;
        p = p->suivant;
    }
    return NULL;
}

void ajouter_livre_liste(const char *titre, const char *auteur,
                         const char *categorie, int annee, int stock) {
    livre *nv = malloc(sizeof(livre));
    if (!nv) { printf("Erreur memoire\n"); return; }
    strcpy(nv->titre,     titre);
    strcpy(nv->auteur,    auteur);
    strcpy(nv->categorie, categorie);
    nv->annee   = annee;
    nv->stock   = stock;
    nv->suivant = NULL;
    if (!biblio) { biblio = nv; return; }
    livre *tmp = biblio;
    while (tmp->suivant) tmp = tmp->suivant;
    tmp->suivant = nv;
}

void ajouter_livre_admin() {
    char titre[100], auteur[50], categorie[50];
    int annee;
    printf("  Titre : ");     scanf("%99[^\n]", titre);     vider_buffer();
    printf("  Auteur : ");    scanf("%49[^\n]", auteur);    vider_buffer();

    livre *existant = chercher_livre(titre, auteur);
    if (existant) {
        existant->stock++;
        printf("  Livre deja present. Stock mis a jour : %d exemplaire(s).\n",
               existant->stock);
        push_recent(titre, auteur);
        return;
    }
    printf("  Categorie : "); scanf("%49[^\n]", categorie); vider_buffer();
    printf("  Annee : ");     scanf("%d", &annee);          vider_buffer();

    ajouter_livre_liste(titre, auteur, categorie, annee, 1);
    push_recent(titre, auteur);
    printf("  Livre ajoute avec succes.\n");
}

void afficher_catalogue() {
    if (!biblio) { printf("  La bibliotheque est vide.\n"); return; }
    printf("\n  %-35s %-25s %-15s %4s  Stock\n",
           "Titre", "Auteur", "Categorie", "An.");
    printf("  %.35s %.25s %.15s %.4s  %.12s\n",
           "-----------------------------------",
           "-------------------------",
           "---------------", "----", "------------");
    livre *p = biblio;
    while (p) {
        printf("  %-35s %-25s %-15s %4d  %s\n",
               p->titre, p->auteur, p->categorie, p->annee,
               p->stock > 0 ? "Disponible" : "Indisponible");
        p = p->suivant;
    }
}

/* ============================================================
   FICHIER — BIBLIOTHEQUE.TXT
   ============================================================ */
void sauvegarder_bibliotheque() {
    FILE *f = fopen("bibliotheque.txt", "w");
    if (!f) { printf("  Erreur : impossible d'ouvrir bibliotheque.txt\n"); return; }
    livre *p = biblio;
    while (p) {
        fprintf(f, "%s|%s|%s|%d|%d\n",
                p->titre, p->auteur, p->categorie, p->annee, p->stock);
        p = p->suivant;
    }
    fclose(f);
}

void charger_bibliotheque() {
    FILE *f = fopen("bibliotheque.txt", "r");
    if (!f) return;
    char ligne[300];
    while (fgets(ligne, sizeof(ligne), f)) {
        ligne[strcspn(ligne, "\n")] = '\0';
        livre *nv = malloc(sizeof(livre));
        if (!nv) continue;
        char *tok;
        tok = strtok(ligne, "|"); if (!tok){free(nv);continue;} strcpy(nv->titre,     tok);
        tok = strtok(NULL,  "|"); if (!tok){free(nv);continue;} strcpy(nv->auteur,    tok);
        tok = strtok(NULL,  "|"); if (!tok){free(nv);continue;} strcpy(nv->categorie, tok);
        tok = strtok(NULL,  "|"); if (!tok){free(nv);continue;} nv->annee = atoi(tok);
        tok = strtok(NULL,  "|"); if (!tok){free(nv);continue;} nv->stock = atoi(tok);
        nv->suivant = NULL;
        if (!biblio) { biblio = nv; continue; }
        livre *tmp = biblio; while (tmp->suivant) tmp = tmp->suivant;
        tmp->suivant = nv;
    }
    fclose(f);
}

/* ============================================================
   FICHIER — UTILISATEURS.TXT
   Format par utilisateur :
     USER|id|nom|points
     HISTO|operation|titre|auteur|jour|mois|annee
     HISTO|...
     END
   ============================================================ */
void sauvegarder_utilisateurs() {
    FILE *f = fopen("utilisateurs.txt", "w");
    if (!f) { printf("  Erreur : impossible d'ouvrir utilisateurs.txt\n"); return; }

    utilisateur *u = liste_users;
    while (u) {
        fprintf(f, "USER|%d|%s|%d\n", u->ID, u->nom, u->points);

        /* Sauvegarder la pile historique dans l'ordre inverse
           pour qu'au rechargement, push_historique() reconstitue
           la pile dans le bon ordre (dernier emprunt en sommet). */
        /* D'abord, compter les elements et les mettre dans un tableau */
        int count = 0;
        pile_historique *p = u->histo_tete;
        while (p) { count++; p = p->suivant; }

        pile_historique **tab = malloc(count * sizeof(pile_historique *));
        if (tab) {
            p = u->histo_tete;
            for (int i = 0; i < count; i++) { tab[i] = p; p = p->suivant; }
            /* Ecrire du plus ancien (fin du tableau) au plus recent */
            for (int i = count - 1; i >= 0; i--) {
                fprintf(f, "HISTO|%s|%s|%s|%d|%d|%d\n",
                        tab[i]->operation,
                        tab[i]->titre,
                        tab[i]->auteur,
                        tab[i]->date_op.jour,
                        tab[i]->date_op.mois,
                        tab[i]->date_op.annee);
            }
            free(tab);
        }
        fprintf(f, "END\n");
        u = u->suivant;
    }
    fclose(f);
}

void charger_utilisateurs() {
    FILE *f = fopen("utilisateurs.txt", "r");
    if (!f) return;

    char ligne[400];
    utilisateur *u_courant = NULL;

    while (fgets(ligne, sizeof(ligne), f)) {
        ligne[strcspn(ligne, "\n")] = '\0';
        if (strlen(ligne) == 0) continue;

        if (strncmp(ligne, "USER|", 5) == 0) {
            /* USER|id|nom|points */
            utilisateur *nv = malloc(sizeof(utilisateur));
            if (!nv) continue;
            nv->histo_tete = NULL;
            nv->suivant    = NULL;
            char *tok;
            tok = strtok(ligne + 5, "|"); if (!tok){free(nv);continue;} nv->ID     = atoi(tok);
            tok = strtok(NULL,      "|"); if (!tok){free(nv);continue;} strcpy(nv->nom, tok);
            tok = strtok(NULL,      "|"); if (!tok){free(nv);continue;} nv->points = atoi(tok);

            if (!liste_users) { liste_users = nv; }
            else {
                utilisateur *tmp = liste_users;
                while (tmp->suivant) tmp = tmp->suivant;
                tmp->suivant = nv;
            }
            u_courant = nv;

        } else if (strncmp(ligne, "HISTO|", 6) == 0 && u_courant) {
            /* HISTO|operation|titre|auteur|jour|mois|annee */
            char op[10], titre[100], auteur[50];
            date d;
            char *tok;
            tok = strtok(ligne + 6, "|"); if (!tok) continue; strcpy(op,     tok);
            tok = strtok(NULL,      "|"); if (!tok) continue; strcpy(titre,  tok);
            tok = strtok(NULL,      "|"); if (!tok) continue; strcpy(auteur, tok);
            tok = strtok(NULL,      "|"); if (!tok) continue; d.jour  = atoi(tok);
            tok = strtok(NULL,      "|"); if (!tok) continue; d.mois  = atoi(tok);
            tok = strtok(NULL,      "|"); if (!tok) continue; d.annee = atoi(tok);
            push_historique(u_courant, titre, auteur, op, d);
        }
        /* "END" : on passe a l'utilisateur suivant (u_courant sera ecrase) */
    }
    fclose(f);
}

/* ============================================================
   FICHIER — HISTORIQUE.TXT  (log global de toutes les operations)
   ============================================================ */
void log_operation(utilisateur *u, const char *op,
                   const char *titre, const char *auteur, date d) {
    FILE *f = fopen("historique.txt", "a");
    if (!f) return;
    fprintf(f, "ID:%-4d | %-20s | [%-7s] \"%s\" — %s | %02d/%02d/%d\n",
            u->ID, u->nom, op, titre, auteur, d.jour, d.mois, d.annee);
    fclose(f);
}

/* ============================================================
   LISTE CHAÎNÉE — UTILISATEURS
   ============================================================ */
utilisateur *chercher_utilisateur(int id) {
    utilisateur *p = liste_users;
    while (p) { if (p->ID == id) return p; p = p->suivant; }
    return NULL;
}

utilisateur *creer_ou_connecter_lecteur() {
    int id;
    char nom[50];
    printf("  Votre ID : "); scanf("%d", &id); vider_buffer();
    utilisateur *u = chercher_utilisateur(id);
    if (u) {
        printf("  Bienvenue, %s ! (Points : %d)\n", u->nom, u->points);
        return u;
    }
    printf("  Nouvel utilisateur. Votre nom : "); scanf("%49[^\n]", nom); vider_buffer();
    utilisateur *nv = malloc(sizeof(utilisateur));
    if (!nv) { printf("Erreur memoire\n"); return NULL; }
    nv->ID         = id;
    nv->points     = 0;
    nv->histo_tete = NULL;
    nv->suivant    = NULL;
    strcpy(nv->nom, nom);
    if (!liste_users) { liste_users = nv; }
    else {
        utilisateur *tmp = liste_users;
        while (tmp->suivant) tmp = tmp->suivant;
        tmp->suivant = nv;
    }
    sauvegarder_utilisateurs();
    printf("  Compte cree. Bienvenue, %s !\n", nv->nom);
    return nv;
}

/* ============================================================
   OPÉRATIONS LECTEUR — EMPRUNTER / RETOURNER
   ============================================================ */
void emprunter(utilisateur *u) {
    char titre[100], auteur[50];
    printf("  Titre du livre : "); scanf("%99[^\n]", titre); vider_buffer();
    printf("  Auteur : ");         scanf("%49[^\n]", auteur); vider_buffer();
    date d = saisir_date();

    livre *l = chercher_livre(titre, auteur);
    if (!l) { printf("  Ce livre n'existe pas dans la bibliotheque.\n"); return; }

    if (l->stock <= 0) {
        enqueue(u->ID, u->nom, titre, auteur);
        return;
    }
    l->stock--;
    date dr = calculer_date_retour(d);
    printf("  Emprunt enregistre. Date de retour prevue : %02d/%02d/%d\n",
           dr.jour, dr.mois, dr.annee);
    push_historique(u, titre, auteur, "emprunt", d);
    log_operation(u, "emprunt", titre, auteur, d);
    sauvegarder_bibliotheque();
    sauvegarder_utilisateurs();
}

void retourner(utilisateur *u) {
    char titre[100], auteur[50];
    printf("  Titre du livre : "); scanf("%99[^\n]", titre); vider_buffer();
    printf("  Auteur : ");         scanf("%49[^\n]", auteur); vider_buffer();
    date d_reel = saisir_date();

    livre *l = chercher_livre(titre, auteur);
    if (!l) { printf("  Ce livre n'est pas dans le catalogue.\n"); return; }

    date d_emprunt;
    if (!chercher_date_emprunt(u, titre, auteur, &d_emprunt)) {
        printf("  Aucun emprunt de ce livre trouve dans votre historique.\n");
        return;
    }
    date d_prevue = calculer_date_retour(d_emprunt);
    l->stock++;

    if (date_superieure(d_reel, d_prevue)) {
        printf("  Retour en RETARD (prevu le %02d/%02d/%d). Penalite : -10 pts\n",
               d_prevue.jour, d_prevue.mois, d_prevue.annee);
        u->points -= 10;
    } else {
        printf("  Retour a temps. Bravo ! Bonus : +10 pts\n");
        u->points += 10;
    }
    printf("  Total points : %d\n", u->points);
    push_historique(u, titre, auteur, "retour", d_reel);
    log_operation(u, "retour", titre, auteur, d_reel);
    sauvegarder_bibliotheque();
    sauvegarder_utilisateurs();
    notifier_file(titre, auteur);
}

/* ============================================================
   MENUS
   ============================================================ */
void menu_admin() {
    int ch;
    do {
        afficher_separateur();
        printf("  [MODE ADMIN]\n");
        printf("  1. Ajouter un livre\n");
        printf("  2. Afficher le catalogue\n");
        printf("  3. Livres recemment ajoutes\n");
        printf("  4. Afficher la file d'attente\n");
        printf("  5. Afficher tous les historiques lecteurs\n");
        printf("  6. Deconnexion\n");
        printf("  Choix : ");
        scanf("%d", &ch); vider_buffer();
        afficher_separateur();
        switch (ch) {
            case 1: ajouter_livre_admin(); sauvegarder_bibliotheque(); break;
            case 2: afficher_catalogue(); break;
            case 3: afficher_pile_recents(); break;
            case 4: afficher_file_attente(); break;
            case 5: {
                utilisateur *p = liste_users;
                if (!p) { printf("  Aucun lecteur enregistre.\n"); break; }
                while (p) { afficher_historique_lecteur(p); p = p->suivant; }
                break;
            }
            case 6: printf("  Deconnexion admin.\n"); break;
            default: printf("  Choix invalide.\n");
        }
    } while (ch != 6);
}

void menu_lecteur(utilisateur *u) {
    int ch;
    do {
        afficher_separateur();
        printf("  [MODE LECTEUR] — %s | Points : %d\n", u->nom, u->points);
        printf("  1. Emprunter un livre\n");
        printf("  2. Retourner un livre\n");
        printf("  3. Mon historique\n");
        printf("  4. Voir le catalogue\n");
        printf("  5. Mes points\n");
        printf("  6. Deconnexion\n");
        printf("  Choix : ");
        scanf("%d", &ch); vider_buffer();
        afficher_separateur();
        switch (ch) {
            case 1: emprunter(u); break;
            case 2: retourner(u); break;
            case 3: afficher_historique_lecteur(u); break;
            case 4: afficher_catalogue(); break;
            case 5: printf("  Vos points : %d\n", u->points); break;
            case 6: printf("  Deconnexion.\n"); break;
            default: printf("  Choix invalide.\n");
        }
    } while (ch != 6);
}

/* ============================================================
   CATALOGUE INITIAL — 12 livres pre-charges au premier demarrage
   ============================================================ */
void initialiser_catalogue() {
    if (biblio != NULL) return; /* deja charge depuis bibliotheque.txt */

    ajouter_livre_liste("Le Petit Prince",              "Antoine de Saint-Exupery", "Roman",        1943, 3);
    ajouter_livre_liste("Les Miserables",               "Victor Hugo",              "Roman",        1862, 2);
    ajouter_livre_liste("L'Etranger",                   "Albert Camus",             "Roman",        1942, 2);
    ajouter_livre_liste("Madame Bovary",                "Gustave Flaubert",         "Roman",        1857, 1);
    ajouter_livre_liste("Germinal",                     "Emile Zola",               "Roman",        1885, 2);
    ajouter_livre_liste("1984",                         "George Orwell",            "Sci-Fi",       1949, 3);
    ajouter_livre_liste("Le Meilleur des Mondes",       "Aldous Huxley",            "Sci-Fi",       1932, 2);
    ajouter_livre_liste("Fahrenheit 451",               "Ray Bradbury",             "Sci-Fi",       1953, 2);
    ajouter_livre_liste("Le Mythe de Sisyphe",          "Albert Camus",             "Philosophie",  1942, 1);
    ajouter_livre_liste("Ainsi Parlait Zarathoustra",   "Friedrich Nietzsche",      "Philosophie",  1883, 1);
    ajouter_livre_liste("Le Langage C",                 "Kernighan et Ritchie",     "Informatique", 1978, 2);
    ajouter_livre_liste("Introduction aux Algorithmes", "Cormen et al.",            "Informatique", 2009, 2);

    sauvegarder_bibliotheque();
    printf("  Catalogue initialise avec 12 livres par defaut.\n");
}

/* ============================================================
   PROGRAMME PRINCIPAL
   ============================================================ */
int main() {
    charger_bibliotheque();
    charger_utilisateurs();
    initialiser_catalogue();

    int ch;
    do {
        afficher_separateur();
        printf("  === BIBLIOTHEQUE NUMERIQUE ===\n");
        printf("  1. Connexion Admin\n");
        printf("  2. Connexion Lecteur\n");
        printf("  3. Quitter\n");
        printf("  Choix : ");
        scanf("%d", &ch); vider_buffer();

        switch (ch) {
            case 1: {
                char mdp[50];
                printf("  Mot de passe : ");
                scanf("%49[^\n]", mdp); vider_buffer();
                if (strcmp(mdp, MOT_DE_PASSE_ADMIN) == 0) menu_admin();
                else printf("  Mot de passe incorrect.\n");
                break;
            }
            case 2: {
                utilisateur *u = creer_ou_connecter_lecteur();
                if (u) menu_lecteur(u);
                break;
            }
            case 3: printf("  Au revoir !\n"); break;
            default: printf("  Choix invalide.\n");
        }
    } while (ch != 3);

    return 0;
}
