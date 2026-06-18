# base746 Radar Project

![Version](https://img.shields.io/badge/version-1.0.0-blue)
![Build](https://img.shields.io/badge/build-passing-green)

---

<details>
<summary>Com fixes</summary>

1. usb fix:  `sudo chmod 666 /dev/ttyACM0`
2. missing arch module: `sudo pacman -S openocd libusb`

</details>


**Description du capteur ou de l'actionneur**

<img width="647" height="662" alt="image" src="https://github.com/user-attachments/assets/b58a8133-2985-4b93-bd07-40410483bfa6" /> 

Le LD2450 est un module de capteur de suivi de cible en mouvement de la série de radars à ondes millimétriques Hilink 24G.
Il comprend un capteur radar 24 GHz extrêmement simplifié et un micrologiciel doté d'un algorithme intelligent. Cette solution est principalement utilisée dans des environnements intérieurs tels que les maisons, les bureaux et les hôtels pour permettre le suivi de la localisation de personnes en mouvement. Le capteur est composé d'une puce radar à ondes millimétriques AloT, d'une antenne microruban haute performance à un émetteur et deux récepteurs, d'un microcontrôleur économique et de circuits auxiliaires périphériques. Le micrologiciel utilise des formes d'onde FMCW et la technologie de traitement du signal avancée propriétaire de la puce radar.
Il prend en charge la sortie série des données de détection, est prêt à l'emploi et peut être facilement intégré à différents environnements intelligents et produits finaux.

---


**Description de la liaison et principe de fonctionnement hardware
type d'alimentation, type de communication**

<img width="527" height="245" alt="image" src="https://github.com/user-attachments/assets/ff627c3b-77b7-4394-ab61-8274e6a6193a" />
<img width="302" height="45" alt="image" src="https://github.com/user-attachments/assets/452edda2-02cf-43f0-ace1-f561530b1136" />


Le module LD2450 communique directement via le port série conformément au protocole prescrit pour la sortie des données de résultats de détection:
https://drive.google.com/drive/folders/1kTt0Z3hjKKrIF3OCIDGdwQ4KotDJ8SGA

Les données de sortie série contiennent jusqu'à trois cibles, leur position et leur vitesse, ainsi que d'autres informations. L'utilisateur peut les utiliser de manière flexible
selon les scénarios d'application spécifiques. La tension d'alimentation du module est de 5 V et la capacité de l'alimentation
d'entrée doit être supérieure à 200 mA. Le niveau de sortie E/S du module est de 3,3 V. Le débit en bauds par défaut du port série est de
256 000, avec 1 bit d'arrêt et sans bit de parité. 5.2.

---


**Schéma et typon de raccordement à la carte DISCO**

La carte d'origine est conçue pour accepter les modules de type Arduino Uno.
<img width="960" height="720" alt="image" src="https://github.com/user-attachments/assets/a7c3e9cc-3f78-4f84-be1b-75d8d045157d" />

Nous avons donc utilisé le modèle Arduino. J'ai ajouté au circuit un connecteur XT30 pour l'alimentation par batterie et un port supplémentaire pour les E/S à usage général (GPIO),
au cas où nous en aurions besoin.

<img width="488" height="500" alt="image" src="https://github.com/user-attachments/assets/ff54028a-3b58-4ad9-a5ed-ded841f2f25b" />

Le circuit imprimé obtenu est simple face, plutôt compact et s'adapte comme prévu.
<img width="578" height="633" alt="image" src="https://github.com/user-attachments/assets/207647aa-081e-4477-a43c-fc3969939cd1" />

---



**Description de la liaison et principe de fonctionnement software bibliothèques utilisées**

Le fichier LD2450.cpp implémente le pilote radar LD2450 : il lit les octets depuis une interface série matérielle (HardwareSerial), analyse les trames radar pour extraire jusqu’à trois cibles (coordonnées x et y, vitesse, résolution spatiale) et les stocke dans un tampon interne accessible via la fonction getTargets(). Il décode également les valeurs signées 16 bits, reconnaît les trames d’accusé de réception (ACK) et fournit des fonctions d’assistance (setSingleTargetMode, setMultiTargetMode, restart, factoryReset) qui construisent et envoient des trames de commande et attendent (avec un délai d’attente de 200 ms) la réception des accusés de réception correspondants. L’analyse consiste en une simple lecture du tampon qui copie les cibles analysées de manière atomique, et la fonction processByte() ajoute les octets entrants et déclenche l’analyse lorsque suffisamment de données sont accumulées.

---


**Description du fonctionnement de l'application réalisée**

- **Fonction :** Lit les données du radar LD2450 via UART, analyse jusqu'à trois cibles et les affiche sous forme de texte et d'une visualisation graphique simple.
- **Matériel / E/S :** Utilise le port `Serial6` (broches `TX_PIN`/`RX_PIN` → `PC6`/`PC7`) pour communiquer avec le module radar LD2450.
- **Interface utilisateur :** Interface à trois onglets basée sur LVGL : l'onglet 1 affiche les étiquettes `target1_label`, `target2_label` et `target3_label` (texte), l'onglet 2 est `radar_viz_container` avec `target_dots[]` (graphique), l'onglet 3 est un espace réservé.
- **Flux d'exécution :** `mySetup()` construit l'interface utilisateur et configure l'UART, puis lance la tâche FreeRTOS `myTask()`. La fonction `myTask()` lit les octets, appelle `radar.processByte()`, obtient les cibles via `radar.getTargets()`, puis met à jour l'interface utilisateur LVGL (étiquettes + `update_target_dots()`).
- **Logique de visualisation :** `update_target_dots()` convertit les coordonnées en millimètres en positions en pixels, les limite à la zone visible et affiche/masque les points ; des coordonnées nulles signifient « aucune cible ».
- **Remarque concernant la concurrence :** Les mises à jour LVGL sont protégées par `lv_lock()` / `lv_unlock()` car LVGL n'est pas thread-safe.



<img width="812" height="535" alt="image" src="https://github.com/user-attachments/assets/e3e86fa7-c3d5-447c-83e7-fd188ab000fd" />

<img width="396" height="480" alt="image" src="https://github.com/user-attachments/assets/8ee2e8ff-4c3c-4399-8e2d-0b0b610d8d8b" />







