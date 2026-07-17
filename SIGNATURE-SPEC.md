---
title: Symbol Signature Specification
date: 2026-07-14
---

# Symbol Signature Specification

::: toc+
- **But** — associer signatures et opcodes aux constructeurs pour les interpréter algébriquement.
- **Signatures, algèbres et folds** — trois définitions : constructeurs, algèbres et morphisme d'interprétation.
- **Modèle** — signatures, plages globales et opcodes locaux denses.
- **API** — créer une signature, ajouter ses symboles et lire leur identité.
- **Exemple arithmétique minimal** — construire et interpréter une expression avec le même fold.
- **Invariants** — unicité, immutabilité, idempotence et compatibilité.
- **Cycle de vie** — portée des handles et stabilité des numéros.
- **Relation avec fold** — frontière entre infrastructure TLIB et algèbres clientes.
- **Non-objectifs** — informations volontairement absentes de la première version.
- **Tests de conformité** — propriétés à vérifier dans la TLIB autonome.
:::

## But

Dans la TLIB historique, un symbole est essentiellement un nom interné utilisé
pour étiqueter un nœud. Il ne précise ni le langage auquel il appartient,
ni l’opération algébrique qu’il représente.

Pour adopter une approche plus algébrique dans la construction, la
transformation et l’interprétation des arbres, il est utile d’associer aux
symboles utilisés comme constructeurs une **signature** et un **opcode**.

La signature identifie le langage auquel appartient le symbole, par
exemple le langage des signaux de Faust. L’opcode identifie le symbole
dans ce langage et permet d’implémenter un dispatch efficace, sans essayer
successivement tous les prédicats `isXXX` comme dans la version historique.

Pour automatiser l’attribution des opcodes, on introduit des objets
`Signature`. Chacun réserve une plage de 256 opcodes, enregistre ses symboles
et leur attribue automatiquement un opcode dense. Un symbole ne peut appartenir
qu’à une seule signature ; son ajout répété dans cette signature est
idempotent.

Ce document occupe l’étage intermédiaire de la documentation. Le README décrit
les invariants historiques supposés acquis — symboles internés, arbres
hash-consés et cycle de vie par session. La présente spécification ajoute
l’identité algébrique des constructeurs. Les bibliothèques clientes, notamment
Signals et Faust, définissent ensuite leurs signatures concrètes, leurs
algèbres et leurs folds. `REWRITE-SPEC.md` complète ce mécanisme en spécifiant
la reconstruction et la transformation des arbres partagés.

## Signatures, algèbres et folds

Le vocabulaire de cette spécification vient de l’algèbre universelle et se
réduit à trois définitions :

Signature
:   un ensemble fini de symboles constructeurs, chacun muni d’une arité. La
    classe TLIB `Signature` matérialise l’ensemble, mais cette première API
    n’enregistre pas l’arité. Celle-ci est définie par l’algèbre cliente et
    vérifiée sur chaque occurrence du constructeur dans un arbre (voir
    *Non-objectifs*).

Algèbre
:   un domaine de valeurs `T`, le support, et une opération sur `T` de même
    arité pour chaque constructeur de la signature. Une signature admet
    autant d’algèbres que d’interprétations utiles : types, intervalles,
    valeurs numériques.

Fold
:   la fonction qui interprète un terme dans une algèbre 𝒜 : elle interprète
    récursivement les branches, puis applique l’opération de 𝒜 correspondant
    au constructeur du nœud[^fold].

[^fold]: Le terme vient de la programmation fonctionnelle : c’est le *fold* des listes généralisé aux arbres, aussi appelé catamorphisme.

```math
⟦c(t₁, …, tₙ)⟧_𝒜 = c_𝒜(⟦t₁⟧_𝒜, …, ⟦tₙ⟧_𝒜)
```

Pour les termes finis non récursifs, les arbres hash-consés forment eux-mêmes
une algèbre particulière de toute signature : l’algèbre des termes, dont
chaque opération est simplement le constructeur d’arbre correspondant. Cette
algèbre est dite initiale : appliquer le fold vers elle reconstruit le terme à
l’identique — et, par hash-consing, redonne le même pointeur — tandis que le
fold vers toute autre algèbre définit l’unique morphisme qui remplace chaque
constructeur par son opération. Les termes récursifs nécessitent en plus une
sémantique de point fixe, qui n’est pas déterminée par la seule signature. Les
deux assertions finales de l’exemple arithmétique expriment exactement les
deux lectures du cas non récursif.

Deux conséquences guident la conception :

- au niveau d’un nœud, le fold n’a besoin que de passer du constructeur $c$ à
  l’opération $c_𝒜$ en temps constant ; c’est précisément ce que fournissent
  la signature et l’opcode dense ;
- la valeur $⟦t⟧_𝒜$ ne dépendant que de $t$, un fold sur un DAG peut
  mémoïser `Tree → T` et interpréter chaque sous-arbre partagé une seule
  fois — la même propriété constructive que `REWRITE-SPEC.md` exploite pour
  la réécriture.

## Modèle

Une signature internée de nom `S` réserve à sa création une plage alignée de
256 opcodes globaux :

```text
range(S) = [base(S), base(S) + 255]
base(S) mod 256 = 0
```

La base n’est pas calculée à partir du nom de la signature. Un compteur global
de session, initialisé à zéro, attribue les plages dans l’ordre de première
création :

```text
signature("Signal")  -> base = 0,   range = [0, 255]
signature("Box")     -> base = 256, range = [256, 511]
signature("Type")    -> base = 512, range = [512, 767]
```

Plus généralement, si `Sᵢ` est la iᵉ signature créée en comptant à partir de
zéro, `base(Sᵢ) = 256 × i`. Retrouver une signature existante ne réserve pas de
nouvelle plage.

Les plages de deux signatures distinctes sont disjointes. `add(name)` alloue
les opcodes locaux `0, 1, …, 255` dans l’ordre des premiers ajouts et stocke
sur le symbole l’opcode global :

```text
opcode(symbol) = base(signature) + localOpcode(symbol)
```

Le type public des opcodes globaux est `std::uint32_t`. L’espace permet donc
plusieurs millions de signatures tout en conservant une capacité exactement
égale à 256 constructeurs par signature.

## API

L’API de construction est volontairement minimale :

```cpp
auto signal = signature("Signal");

Sym input  = signal.add("SigInput");
Sym delay1 = signal.add("SigDelay1");
Sym delay  = signal.add("SigDelay");
Sym binop  = signal.add("SigBinOp");
```

`Signature` est un handle copiable sur un état interné appartenant à la session
TLIB. Deux appels `signature(name)` avec le même nom retournent des handles vers
la même signature et poursuivent la même allocation dense.

Le fold et les outils de diagnostic peuvent lire l’association sans pouvoir la
modifier directement :

```cpp
using SymbolOpcode = std::uint32_t;
inline constexpr SymbolOpcode kOpcodesPerSignature = 256;

struct SymbolTag {
    Sym          signature;
    SymbolOpcode opcode;

    constexpr std::uint8_t localOpcode() const noexcept;
};

bool getSymbolTag(Sym symbol, SymbolTag& tag);
```

`Signature::identity()` expose le `Sym` interné qui identifie la signature afin
de vérifier `tag.signature == signal.identity()`. L’API brute
`registerSymbolTag` disparaît : laisser un client choisir lui-même l’opcode
permettrait de violer la densité et la disjonction des plages.

## Exemple arithmétique minimal

Considérons une signature arithmétique avec quatre constructeurs binaires. Cette
signature induit une même interface algébrique pour chaque domaine de résultats
`T`. Les signatures des méthodes encodent ici l’arité que la classe TLIB
`Signature` ne stocke pas :

```cpp
template <typename T>
class ArithmeticAlgebra {
public:
    using Value = T;

    virtual ~ArithmeticAlgebra() = default;

    // Number interprets native TLIB numeric atoms; the other methods
    // interpret the four constructors registered in the signature.
    virtual T Number(double x) = 0;
    virtual T Add(T x, T y) = 0;
    virtual T Sub(T x, T y) = 0;
    virtual T Mul(T x, T y) = 0;
    virtual T Div(T x, T y) = 0;
};
```

L’algèbre primitive réalise cette interface sur les `Tree`. Elle possède la
signature et ses symboles, construit les termes libres et fournit leur fold
vers toute autre algèbre arithmétique. Les noms `Arithmetic.Add`,
`Arithmetic.Sub`, `Arithmetic.Mul` et `Arithmetic.Div` sont qualifiés parce que
tous les symboles TLIB partagent le même espace de noms interné :

```cpp
class ArithmeticTreeAlgebra : public ArithmeticAlgebra<Tree> {
private:
    Signature fSignature = signature("Arithmetic");
    Sym fAdd = fSignature.add("Arithmetic.Add");
    Sym fSub = fSignature.add("Arithmetic.Sub");
    Sym fMul = fSignature.add("Arithmetic.Mul");
    Sym fDiv = fSignature.add("Arithmetic.Div");

public:
    Tree Number(double x) override    { return tree(x); }
    Tree Add(Tree x, Tree y) override { return tree(fAdd, x, y); }
    Tree Sub(Tree x, Tree y) override { return tree(fSub, x, y); }
    Tree Mul(Tree x, Tree y) override { return tree(fMul, x, y); }
    Tree Div(Tree x, Tree y) override { return tree(fDiv, x, y); }

    /**
     * Interpret a valid primitive arithmetic term in \p algebra.
     *
     * Numeric atoms are injected directly; binary nodes are checked against
     * this signature, folded bottom-up, then dispatched by dense local opcode.
     */
    template <typename Algebra>
    typename Algebra::Value fold(Tree expression, Algebra& algebra) const
    {
        double number;
        if (isDouble(expression->node(), &number)) {
            return algebra.Number(number);
        }

        Sym constructor;
        SymbolTag tag;
        if (!isSym(expression->node(), &constructor) ||
            !getSymbolTag(constructor, tag) ||
            tag.signature != fSignature.identity() ||
            expression->arity() != 2) {
            tlib::error("invalid arithmetic expression");
        }

        auto x = fold(expression->branch(0), algebra);
        auto y = fold(expression->branch(1), algebra);

        // localOpcode() exposes the dense position without duplicating the
        // representation of aligned 256-opcode ranges in every client.
        switch (tag.localOpcode()) {
            case 0: return algebra.Add(x, y);
            case 1: return algebra.Sub(x, y);
            case 2: return algebra.Mul(x, y);
            case 3: return algebra.Div(x, y);
            default: tlib::error("unknown arithmetic opcode");
        }
    }
};
```

L’algèbre d’évaluation réalise la même interface sur les nombres :

```cpp
class ArithmeticEvalAlgebra : public ArithmeticAlgebra<double> {
public:
    double Number(double x) override        { return x; }
    double Add(double x, double y) override { return x + y; }
    double Sub(double x, double y) override { return x - y; }
    double Mul(double x, double y) override { return x * y; }
    double Div(double x, double y) override { return x / y; }
};
```

Le fold part ainsi explicitement de l’algèbre initiale :

```cpp
ArithmeticTreeAlgebra syntax;
ArithmeticEvalAlgebra evaluation;

Tree expression =
    syntax.Mul(syntax.Add(syntax.Number(2), syntax.Number(3)),
               syntax.Number(4));

assert(syntax.fold(expression, syntax) == expression);
assert(syntax.fold(expression, evaluation) == 20);

Tree allOperations = syntax.Div(
    syntax.Mul(syntax.Add(syntax.Number(2), syntax.Number(3)),
               syntax.Sub(syntax.Number(10), syntax.Number(2))),
    syntax.Number(2));

assert(syntax.fold(allOperations, syntax) == allOperations);
assert(syntax.fold(allOperations, evaluation) == 20);
```

La première égalité exprime la reconstruction du terme par l’algèbre primitive.
La seconde est le morphisme d’interprétation vers l’algèbre des nombres. La
seconde expression exerce également `Sub` et `Div`, de sorte que les quatre
constructeurs enregistrés soient couverts. Une implémentation complète peut
remplacer le `switch` par une table dense et mémoïser les sous-arbres partagés
sans modifier les interfaces algébriques. La version exécutable de cet exemple
est `checkArithmeticSignatureFold()` dans `tests.cpp`.

## Invariants

1. Un symbole ordinaire créé par `symbol(name)` reste non signé tant qu’aucune
   signature ne l’ajoute.
2. Un symbole constructeur appartient à une seule signature.
3. Son association et son opcode sont immuables jusqu’à la fin de la session.
4. Répéter `S.add(name)` retourne le même `Sym` et ne consomme aucun opcode.
5. Ajouter à `S₂` un symbole déjà ajouté à `S₁`, avec `S₁ != S₂`, échoue sans
   modifier l’association existante.
6. Les 256 premiers symboles distincts d’une signature reçoivent une suite
   dense ; le 257e est rejeté.
7. Le tag reste indépendant de `getUserData()` et `setUserData()`.
8. Une erreur passe par le gestionnaire TLIB et ne laisse aucun état partiel.
9. L’identité d’une signature appartient au même espace de noms interné que
   les symboles ordinaires. Ce rôle est indépendant du rôle de constructeur :
   le même `Sym` peut assurer les deux sans modifier son unique appartenance
   éventuelle comme constructeur.
10. Lorsque l’espace global de 32 bits ne peut plus réserver une plage complète,
    la création d’une nouvelle signature échoue avant d’interner son identité
    ou de modifier le registre.

## Cycle de vie

Les signatures suivent le modèle de session de TLIB. `tlib::cleanup()` invalide
les `Tree`, les `Sym` et les handles `Signature`, puis remet à zéro le registre
et l’allocation des plages. Une nouvelle session peut réutiliser les mêmes noms.

Les opcodes globaux dépendent de l’ordre de création des signatures et de
l’ordre de leurs premiers ajouts. Ils sont stables pendant une session mais ne
constituent ni un format de fichier ni une ABI persistante. Un client doit
reconstruire ses signatures à chaque initialisation.

Un opcode sur 32 bits autorise exactement 2²⁴ plages de 256 valeurs. Une
signature existante reste accessible après épuisement ; seule la réservation
d’une nouvelle plage est rejetée. Ce cas limite est spécifié, mais une suite de
tests ordinaire ne crée pas artificiellement plus de seize millions de
signatures pour le provoquer.

Comme le reste des tables globales de TLIB, l’enregistrement n’introduit aucune
garantie nouvelle d’accès concurrent pendant l’initialisation.

## Relation avec fold

La signature rend possible un dispatch local en temps constant :

```text
Tree → Sym → SymbolTag(signature, opcode) → opération de l’algèbre
```

Un fold vérifie d’abord que `tag.signature` est celle annoncée par l’algèbre,
puis utilise l’opcode local comme indice de table, ou l’opcode global comme clé
de dispatch. Le même arbre peut ainsi être interprété par une algèbre
syntaxique, un calcul de type, un calcul d’intervalle ou un évaluateur sans
dupliquer le parcours.

TLIB fournit l’identité des constructeurs et pourra fournir le parcours
mémoïsé. La bibliothèque cliente conserve la définition de sa signature, la
table `opcode → opération`, les sortes statiques et toute politique de point
fixe nécessaire aux graphes récursifs.

## Non-objectifs

Cette première version n’encode pas :

- l’arité, déjà portée par le `Tree` et vérifiée par l’algèbre cliente ;
- les sortes des arguments d’une signature multisorte ;
- un pointeur de fonction ou une méthode de dispatch dans le symbole ;
- un espace de noms distinct pour les identités de signatures ;
- la sérialisation des opcodes globaux ;
- le fold lui-même ;
- la stratégie de point fixe des termes récursifs.

## Tests de conformité

- retrouver la même signature par son nom ;
- allouer des opcodes locaux denses dans l’ordre des premiers ajouts ;
- répéter un ajout sans changer l’opcode suivant ;
- obtenir l’opcode local par `SymbolTag::localOpcode()` sans masque client ;
- vérifier la disjonction de deux plages ;
- rejeter l’ajout d’un symbole à une seconde signature sans altérer son tag ;
- accepter exactement 256 constructeurs et rejeter le 257e ;
- laisser les symboles ordinaires non signés ;
- réutiliser un symbole ordinaire comme identité de signature et combiner les
  rôles d’identité et de constructeur ;
- préserver indépendamment `fData` ;
- remettre le registre et les plages à zéro après `tlib::cleanup()`.
