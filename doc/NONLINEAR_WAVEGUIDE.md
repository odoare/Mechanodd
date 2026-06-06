# NONLINEAR_WAVEGUIDE.md

Note de design pour l'extension non-linéaire du modèle de corde par guides d'onde (mechanodd).

Cible : implémentation JUCE C++ temps réel, sur une structure existante de plusieurs lignes de
retard modélisant la propagation entre points d'une corde.

Deux non-linéarités sont visées :

1. **Raidissement aux grandes amplitudes** (hardening) — augmentation de la fréquence avec
   l'amplitude, donc réduction effective des délais.
2. **Chocs / contact unilatéral** — scission de la corde en deux zones couplées lorsque le
   déplacement franchit un seuil signé (butée, frette, chevalet, obstacle).

---

## 1. Rappel du cadre waveguide

Une corde idéale se modélise par deux ondes voyageant en sens opposés (d'Alembert), chacune
portée par une ligne de retard. La fréquence fondamentale est fixée par la longueur totale de
boucle `N` (en échantillons) :

```
f0 = fs / (2 * N)
```

Les deux non-linéarités ci-dessous agissent sur cette structure :
- le **hardening** module la *longueur* de boucle `N` en fonction de l'amplitude ;
- la **collision** insère une *jonction de scattering* à un point interne, scindant la ligne.

Référence de fond : Julius O. Smith, *Physical Audio Signal Processing*,
<https://ccrma.stanford.edu/~jos/pasp/> (chapitres « Vibrating Strings » et « Nonlinear
Elements »).

---

## 2. Couche 1 — Raidissement par tension modulée (hardening)

### 2.1 Principe physique

Quand la corde vibre à grande amplitude, son allongement géométrique augmente la tension, donc
la célérité des ondes, donc la fréquence. C'est le mécanisme de *tension modulation* : la
fréquence monte transitoirement aux fortes amplitudes (« pitch glide » du pluck fort), puis
redescend quand l'énergie décroît.

### 2.2 Référence d'implémentation

Tolonen, Välimäki, Karjalainen, *« Modeling of Tension Modulation Nonlinearity in Plucked
Strings »*, IEEE Transactions on Speech and Audio Processing, 8(3), 2000. C'est la référence
directe pour une implémentation waveguide efficace.

Idée centrale : on n'a pas besoin de résoudre la tension localement partout. On estime une
**élongation globale** de la corde à partir de son profil de déplacement, on en déduit une
variation relative de tension, et on module la longueur effective de la ligne de retard.

### 2.3 Formulation

L'élongation relative (déviation de longueur par rapport à la corde au repos) :

```
L_string = ∫ sqrt( 1 + (∂y/∂x)² ) dx   ≈   L0 + (1/2) ∫ (∂y/∂x)² dx
epsilon  = (L_string − L0) / L0
```

En discret, sur les `M` points de jonction de déplacement `y[i]` espacés de `dx` :

```
sum_slope2 = Σ_i ( (y[i+1] − y[i]) / dx )²
epsilon    = 0.5 * dx * sum_slope2 / L0
```

Facteur de tension (linéarisé, `alpha` = coefficient de couplage à régler) :

```
k_tension = 1 + alpha * epsilon          (k_tension ≥ 1)
```

La célérité varie en `sqrt(k_tension)`, donc la longueur de boucle effective :

```
N_eff = N0 / sqrt(k_tension)
```

`N_eff` est en général **fractionnaire** → délai fractionnaire requis (cf. §2.5).

### 2.4 Lissage obligatoire (anti-zipper)

`epsilon` change à chaque échantillon : moduler `N_eff` brutalement crée du *zipper noise* et
des transitoires. Il faut lisser le paramètre de modulation avec un one-pole :

```
epsilon_smooth += g * (epsilon − epsilon_smooth)
```

avec `g` correspondant à un temps de réponse de ~1–10 ms (`g = 1 − exp(−1/(tau*fs))`).
Le lissage se fait sur des membres déjà alloués — aucune allocation dans le bloc audio.

### 2.5 Délai fractionnaire

Pour réaliser `N_eff` non entier, deux options classiques (toutes deux traitées chez Välimäki) :

- **Interpolation de Lagrange** (ordre 1 à 3) : simple, stable, peu coûteuse. Recommandée pour
  démarrer. Attention à l'atténuation HF en fonction de la partie fractionnaire.
- **Allpass de Thiran** : préserve mieux le module mais introduit un transitoire à chaque
  changement de coefficient → demande un lissage prudent des coefficients. À réserver à une
  v2 si la qualité de l'interpolation Lagrange est insuffisante.

### 2.6 Esquisse C++ (hors classe, à intégrer dans la boucle audio)

```cpp
// Membres pré-alloués (état persistant) :
//   float epsilonSmooth = 0.0f;
//   float smoothingCoeff;   // = 1 - std::exp(-1.0f / (tauSeconds * sampleRate))
//   float alpha;            // couplage tension (réglable, ~ qq unités)
//   double N0;              // longueur de boucle au repos (échantillons)
//   const float dx, invL0;  // pas spatial, 1/L0 précalculés

// Estimation d'élongation à partir des déplacements aux jonctions :
float sumSlope2 = 0.0f;
for (int i = 0; i < numJunctions - 1; ++i)
{
    const float slope = (y[i + 1] - y[i]) * invDx;   // invDx = 1/dx précalculé
    sumSlope2 += slope * slope;
}
const float epsilon = 0.5f * dx * sumSlope2 * invL0;

// Lissage (lock-free, sans allocation) :
epsilonSmooth += smoothingCoeff * (epsilon - epsilonSmooth);

// Facteur de tension et longueur effective :
const float kTension = 1.0f + alpha * epsilonSmooth;       // >= 1
const double Neff     = N0 / std::sqrt((double) kTension);

// -> lire la ligne de retard à la longueur fractionnaire Neff
//    via interpolation Lagrange (cf. §2.5)
```

### 2.7 Notes JUCE temps réel

- Aucune allocation, aucun lock, aucune exception dans `processBlock`.
- `alpha`, `tau`, etc. exposés en `AudioParameterFloat` via l'APVTS ; lire les valeurs atomiques
  une fois par bloc et lisser à l'intérieur du bloc.
- Précalculer `invDx`, `invL0`, `smoothingCoeff` dans `prepareToPlay`.
- Le profil `y[]` aux jonctions est déjà disponible dans la structure multi-lignes existante ;
  ne pas le recopier inutilement.

---

## 3. Couche 2 — Chocs / contact unilatéral (collision)

### 3.1 Principe physique

Un obstacle (butée, frette, chevalet, corde voisine) limite le déplacement d'un côté. Quand le
déplacement franchit la position de l'obstacle (seuil **signé**), la corde entre en contact :
elle se comporte localement comme scindée en deux waveguides couplés de part et d'autre du point
de contact. C'est un *contact unilatéral*.

### 3.2 Références d'implémentation

- Bilbao, Torin, Chatziioannou, *« Numerical Modeling of Collisions in Musical Instruments »*,
  Acta Acustica united with Acustica, 2015 — schémas énergie-conservatifs (référence moderne).
- Chatziioannou, van Walstijn, *« Energy conserving schemes for the simulation of musical
  instrument contact dynamics »*, Journal of Sound and Vibration, 2015.
- Evangelista, Eckerholm, *« Player–Instrument Interaction Models for Digital Waveguide
  Synthesis of Guitar: Touch and Collisions »*, IEEE TASLP, 2010 — traite **spécifiquement** la
  scission d'un waveguide par contact (touch, fret, capo), donc le plus proche de notre cas.

### 3.3 Le piège : ne pas utiliser un seuil dur

Un commutateur binaire (`if (y > seuil) { ... }`) crée des discontinuités → clics, énergie non
physique, instabilité. Il faut une **force de contact continue**, typiquement une pénalité de
type Hertz :

```
f_contact = K * [ y − y_seuil ]_+ ^ beta
```

où `[x]_+ = max(x, 0)` (partie positive, d'où le caractère **signé** du seuil), `K` la raideur
de contact, `beta` l'exposant (1.0 = ressort linéaire ; 1.5–3.0 = contact de Hertt plus réaliste
et plus doux à l'engagement). La force agit en réaction sur le déplacement au point de contact.

### 3.4 Réalisation waveguide : jonction de scattering variable

Le point de contact devient une **jonction de scattering** dont l'« ouverture » varie
continûment avec la force de contact :
- contact nul → jonction transparente, la corde reste une seule ligne ;
- contact fort → la jonction réfléchit fortement, scindant la propagation en deux zones.

En pratique, on insère au point d'indice `p` une terminaison partielle dont le coefficient de
réflexion `rho` est une fonction continue de `f_contact` (montant de 0 à une valeur < 1). Les
ondes incidentes gauche/droite `y⁺`, `y⁻` sont recombinées via les relations de scattering, et la
force de contact ajoute une composante au point `p`. Evangelista (2010) donne les relations
exactes de la jonction variable dans le temps.

### 3.5 Esquisse C++ (force de contact)

```cpp
// Membres pré-alloués :
//   float contactK;        // raideur de contact
//   float contactBeta;     // exposant (1.0 .. 3.0)
//   float yThreshold;      // seuil signé (position de l'obstacle)
//   int   contactIndex;    // point p où l'obstacle est placé

const float penetration = y[contactIndex] - yThreshold;  // signé
if (penetration > 0.0f)
{
    const float fContact = contactK * std::pow(penetration, contactBeta);
    // appliquer fContact en réaction au point contactIndex,
    // et/ou piloter le coefficient de réflexion rho de la jonction (cf. §3.4)
    // rho croît avec fContact, borné < 1 pour la stabilité.
}
// penetration <= 0 : pas de contact, jonction transparente.
```

> `std::pow` avec exposant fractionnaire est coûteux en temps réel. Si `beta` est fixe (ex. 1.0
> ou 1.5), spécialiser : `beta=1` → multiplication simple ; `beta=1.5` → `x * sqrt(x)`. Éviter
> `std::pow` générique dans la boucle chaude.

### 3.6 Stabilité

- La raideur `K` et le pas temporel imposent une limite de stabilité (contact raide = système
  rapide). Commencer avec `K` modéré et augmenter prudemment.
- Pour un contact très raide, les schémas énergie-conservatifs de Bilbao/Chatziioannou évitent
  l'explosion numérique ; ils sont plus lourds qu'une simple pénalité mais robustes.
- Borner `rho < 1` strictement.

---

## 4. Architecture proposée pour mechanodd

Deux couches indépendantes, activables séparément, greffées sur les lignes de retard existantes :

| Couche | Action sur la structure | Pilotée par | Coût |
|---|---|---|---|
| Hardening (§2) | Module la longueur fractionnaire de boucle | Élongation globale lissée | Faible |
| Collision (§3) | Jonction de scattering variable à un point | Force de contact continue | Moyen |

Ordre d'implémentation recommandé :

1. **Hardening d'abord** — peu coûteux, très musical, valide la chaîne de délai fractionnaire.
   Brique de base : tension modulation de Tolonen sur une seule ligne de retard, puis extension
   à la structure multi-lignes.
2. **Collision ensuite** — commencer par une pénalité Hertzienne à un point fixe avec `beta`
   fixe, jonction de réflexion simple ; raffiner vers la jonction de scattering variable
   d'Evangelista si besoin.

### 4.1 Paramètres exposés (APVTS)

- Hardening : `hardening_amount` (alpha), `hardening_smoothing` (tau ms), `enable_hardening`.
- Collision : `contact_position` (point p, normalisé 0–1), `contact_threshold` (seuil signé),
  `contact_stiffness` (K), `contact_exponent` (beta), `enable_collision`.

### 4.2 Garde-fous temps réel (rappel SKILL.md)

- `processBlock` : pas d'allocation, pas de lock, pas d'exception, pas de `std::pow` générique
  en boucle chaude.
- Lissage de tous les paramètres modulant un délai ou une réflexion (anti-zipper).
- Précalculs dans `prepareToPlay` ; lecture atomique des paramètres une fois par bloc.
- Tester chaque couche isolément (bypass de l'autre) avant de les combiner.

---

## 5. Pistes de validation

- **Hardening** : exciter à amplitude croissante, vérifier le pitch glide montant transitoire et
  son retour à `f0` quand l'énergie décroît. Mesurer la dérive de fréquence vs `alpha`.
- **Collision** : vérifier l'absence de clic à l'engagement du contact (continuité de la force),
  l'apparition d'harmoniques/buzz caractéristiques du contact, et la stabilité quand `K` monte.
- Surveiller l'énergie totale : elle ne doit pas croître spontanément (signe d'instabilité).

---

## 6. Références (récapitulatif)

- J. O. Smith, *Physical Audio Signal Processing*, CCRMA, <https://ccrma.stanford.edu/~jos/pasp/>.
- T. Tolonen, V. Välimäki, M. Karjalainen, *Modeling of Tension Modulation Nonlinearity in
  Plucked Strings*, IEEE TSAP 8(3), 2000.
- S. Bilbao, A. Torin, V. Chatziioannou, *Numerical Modeling of Collisions in Musical
  Instruments*, Acta Acustica united with Acustica, 2015.
- V. Chatziioannou, M. van Walstijn, *Energy conserving schemes for the simulation of musical
  instrument contact dynamics*, JSV, 2015.
- G. Evangelista, F. Eckerholm, *Player–Instrument Interaction Models for Digital Waveguide
  Synthesis of Guitar: Touch and Collisions*, IEEE TASLP, 2010.
- (Stiffness/dispersion, si besoin ultérieur) J. Rauhala, V. Välimäki, *Tunable Dispersion
  Filter Design for Piano Synthesis*, IEEE SPL, 2006 ; J. Bensa et al., *The simulation of piano
  string vibration*, JASA, 2003.
