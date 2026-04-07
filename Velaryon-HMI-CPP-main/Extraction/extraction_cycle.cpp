/******************************************************************************************
**                                                                                       **
**   ExtractionTab generates the main content for the "Extraction" tab in the user       **
**   interface. This tab will (hopefully) later allow users to select sensors,           **
**   tests, and time ranges to export data from the database into CSV files.             **
**                                                                                       **
******************************************************************************************/

#include "extraction_cycle.h"

#include "tools/logo.h"
#include "tools/closeButton.h"
#include "tools/scaler.h"
#include "UDP/udpreceiver.h"
#include <QFileDialog>
#include <QStandardPaths>

QWidget* ExtractionTab(QWidget* parent, UdpReceiver& receiver, DatabaseManager* dbManager)
{
    QWidget* extraction_tab = new QWidget(parent);

    //-------------------------------------------------------------------------
    // Layout principal
    QVBoxLayout* mainLayout = new QVBoxLayout(extraction_tab);

    //------------------------------------------------------------------------
    // --- Bandeau supérieur ---
    QHBoxLayout* topBar = new QHBoxLayout();

    QWidget* logoWidget = logo(extraction_tab);
    QWidget* closeBtn = createCloseButton(extraction_tab);
    closeBtn->setMinimumSize(scale_x(120), scale_x(40));

    topBar->addWidget(logoWidget, 0, Qt::AlignLeft);
    topBar->addWidget(closeBtn, 0, Qt::AlignRight);

    //-------------------------------------------------------------------------
    // --- Bloc Capteurs ---
    QGroupBox* sensorBox = new QGroupBox("Sélection des capteurs", extraction_tab);
    QVBoxLayout* sensorLayout = new QVBoxLayout(sensorBox);

    // Création du bouton tous les capteurs
    QCheckBox* allSensorsCheckBox = new QCheckBox("Prendre tous les capteurs", sensorBox);
    allSensorsCheckBox->setChecked(false);
    sensorLayout->addWidget(allSensorsCheckBox);

    // Création du Tree pour choisir les sensors
    QTreeWidget* sensorTree = new QTreeWidget(sensorBox);
    sensorTree->setHeaderHidden(true);
    sensorTree->setMinimumHeight(scale_x(290));

    // Connexion du bouton tous les capteurs au Tree
    QObject::connect(allSensorsCheckBox, &QCheckBox::checkStateChanged, extraction_tab,
    [=](Qt::CheckState state){
        setAllSensorsState(sensorTree, state);
    });

    // Catégorie : Thermocouples
    QTreeWidgetItem* thermoCategory = new QTreeWidgetItem(sensorTree);
    thermoCategory->setText(0, "Thermocouples");

    QStringList thermocouples = {"TS11", "TS12", "TS41", "TS42", "TS61", "TS62"};
    for (const QString& capteur : thermocouples) {
        QTreeWidgetItem* item = new QTreeWidgetItem(thermoCategory);
        item->setText(0, capteur);
        item->setCheckState(0, Qt::Unchecked);
    }

    // Catégorie : Capteurs de pression
    QTreeWidgetItem* pressureCategory = new QTreeWidgetItem(sensorTree);
    pressureCategory->setText(0, "Capteurs de pression");

    QStringList pressures = {"PS11", "PS12", "PS21", "PS22","PS23", "PS31", "PS41", "PS42", "PS51", "PS61", "PS62", "PS63", "PS64", "PS71", "PS81"};
    for (const QString& capteur : pressures) {
        QTreeWidgetItem* item = new QTreeWidgetItem(pressureCategory);
        item->setText(0, capteur);
        item->setCheckState(0, Qt::Unchecked);
    }

    // Catégorie : Débitmètres
    QTreeWidgetItem* flowCategory = new QTreeWidgetItem(sensorTree);
    flowCategory->setText(0, "Débitmètres");

    QStringList flows = {"FM11", "FM21", "FM61"};
    for (const QString& capteur : flows) {
        QTreeWidgetItem* item = new QTreeWidgetItem(flowCategory);
        item->setText(0, capteur);
        item->setCheckState(0, Qt::Unchecked);
    }

    // Catégorie : Capteur de Force
    QTreeWidgetItem* forceCategory = new QTreeWidgetItem(sensorTree);
    forceCategory->setText(0, "Capteur de Force");

    QStringList forces = {"FS01"};
    for (const QString& capteur : forces) {
        QTreeWidgetItem* item = new QTreeWidgetItem(forceCategory);
        item->setText(0, capteur);
        item->setCheckState(0, Qt::Unchecked);
    }

    // Catégorie : Glowplug
    QTreeWidgetItem* GPCategory = new QTreeWidgetItem(sensorTree);
    GPCategory->setText(0, "Courant de la glowplug");

    QStringList GP = {"CM81"};
    for (const QString& capteur : GP) {
        QTreeWidgetItem* item = new QTreeWidgetItem(GPCategory);
        item->setText(0, capteur);
        item->setCheckState(0, Qt::Unchecked);
    }

    // Catégorie : Autres — capteurs présents en DB mais non listés ci-dessus
    // Les noms DB des capteurs déjà listés (après mapping UI → DB)
    QSet<QString> knownDbColumns = {
        "TS11","TS12","TS41","TS42","TS61","TS62",
        "PS11","PS12","PS21","PS22","PS23","PS31",
        "PS41","PS42","PS51","PS61","PS62","PS63","PS64","PS71","PS81",
        "FM11","FM21","FM61",
        "LC",        // FS01
        "glowplug"   // CM81
    };
    //Créer une liste des capteurs pas présen dans ceux déjà cités dans knownDbColumns (donc ceux des autres catégories)
    QStringList otherColumns;
    for (const QString& col : DatabaseManager::allSensorColumns()) {
        if (!knownDbColumns.contains(col))
            otherColumns << col;
    }
    //Créer vraiment l'autre catégorie dans le Tree
    if (!otherColumns.isEmpty()) {
        QTreeWidgetItem* OtherCategory = new QTreeWidgetItem(sensorTree);
        OtherCategory->setText(0, "Autres");
        for (const QString& capteur : otherColumns) {
            QTreeWidgetItem* item = new QTreeWidgetItem(OtherCategory);
            item->setText(0, capteur);
            item->setCheckState(0, Qt::Unchecked);
        }
    }

    //Permet de cocher allSensorsCheckBox si tous les capteurs sont activés et de le décocher si il y en un de décoché
    // Quand un capteur individuel change d'état
    QObject::connect(sensorTree, &QTreeWidget::itemChanged, extraction_tab,
    [=](QTreeWidgetItem* item, int column) {
    if (!item->parent())
        return; // on s'intéresse aux enfants = les capteurs

    // Si un capteur est décoché manuellement
    if (item->checkState(0) == Qt::Unchecked) {
        QSignalBlocker blocker(allSensorsCheckBox); //on empêche allSensors de décocher tous les boutons car il s'est fait décocher                                                                                                                         (il est jaloux) ((d'ailleurs celui qui voit ce message je lui paye une pinte)
        allSensorsCheckBox->setChecked(false);
        return;
    }

    // Vérifier si tous les capteurs sont cochés
    bool allChecked = true;
    for (int i = 0; i < sensorTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* category = sensorTree->topLevelItem(i);
        for (int j = 0; j < category->childCount(); ++j) {
            if (category->child(j)->checkState(0) != Qt::Checked) {
                allChecked = false;
                break;
            }
        }
        if (!allChecked)
        break;
        }

        if (allChecked) {
            QSignalBlocker blocker(allSensorsCheckBox);
            allSensorsCheckBox->setChecked(true);
            }
    });

    sensorLayout->addWidget(sensorTree);
    sensorBox->setLayout(sensorLayout);

    //---------------------------------------------------------------------------
    // --- Bloc Mode de sélection (Test ou Plage horaire) ---
    QGroupBox* modeBox = new QGroupBox("Mode de sélection", extraction_tab);
    QVBoxLayout* modeLayout = new QVBoxLayout(modeBox);

    QRadioButton* modeTest = new QRadioButton("Sélection par test");
    QRadioButton* modeTime = new QRadioButton("Sélection par plage horaire");

    modeTest->setChecked(true); // mode par défaut

    modeLayout->addWidget(modeTest);
    modeLayout->addWidget(modeTime);

    //------------------------------------------------------------------------
    // --- Conteneur dynamique : Test ou Plage horaire ---
    QStackedWidget* modeStack = new QStackedWidget(extraction_tab);

    //------------------------------------------------------------------------
    // --- Sélection par test ---
    QWidget* testPage = new QWidget(extraction_tab);
    QVBoxLayout* testPageLayout = new QVBoxLayout(testPage);

    QGroupBox* testBox = new QGroupBox("Paramètres du test", testPage);
    QVBoxLayout* testLayout = new QVBoxLayout(testBox);

    QLabel* testLabel = new QLabel("Tests disponibles :", testBox);
    QTreeWidget* testList = new QTreeWidget(testBox);
    testList->setHeaderHidden(true);
    testList->setSelectionMode(QAbstractItemView::NoSelection);
    testList->setMinimumHeight(scale_x(200));

    // Liste des tests depuis la base de données
    if (dbManager) {
        const auto dbTests = dbManager->getTestList();
        for (const auto& t : dbTests) {
            QTreeWidgetItem* item = new QTreeWidgetItem(testList);
            item->setText(0, t.name);
            item->setCheckState(0, Qt::Unchecked);
            item->setData(0, Qt::UserRole, t.id);          // ID stocké pour l'export
            item->setData(0, Qt::UserRole + 1, t.startedAt); // startedAt stocké pour affichage
            // Tooltip : dates de début / fin
            QString tip = t.startedAt + " → " + (t.endedAt.isEmpty() ? "En cours" : t.endedAt);
            item->setToolTip(0, tip);
        }
    }

    testLayout->addWidget(testLabel);
    testLayout->addWidget(testList);

    // ---- Auto-refresh du tree quand un test est créé/terminé ----
    if (dbManager) {
        QObject::connect(dbManager, &DatabaseManager::testListChanged, extraction_tab, [=]() {
            // Sauvegarde des tests cochés
            QSet<int> checkedIds;
            for (int i = 0; i < testList->topLevelItemCount(); ++i) {
                QTreeWidgetItem* item = testList->topLevelItem(i);
                if (item && item->checkState(0) == Qt::Checked)
                    checkedIds.insert(item->data(0, Qt::UserRole).toInt());
            }

            // Vider et re-remplir
            testList->clear();
            const auto dbTests = dbManager->getTestList();
            for (const auto& t : dbTests) {
                QTreeWidgetItem* item = new QTreeWidgetItem(testList);
                // Restaurer l'état coché si le test était déjà coché
                item->setCheckState(0, checkedIds.contains(t.id) ? Qt::Checked : Qt::Unchecked);
                item->setData(0, Qt::UserRole, t.id);
                item->setData(0, Qt::UserRole + 1, t.startedAt);

                // Affichage du nom + état (en cours / interrompu / terminé)
                QString status;
                QString tipEnd;
                if (t.endedAt.isEmpty()) {
                    status = " [En cours]";
                    tipEnd = "En cours";
                } else if (t.endedAt.startsWith("INTERRUPTED:")) {
                    status = " [Interrompu]";
                    tipEnd = t.endedAt.mid(12); // date après le préfixe
                } else {
                    status = " [Terminé]";
                    tipEnd = t.endedAt;
                }
                item->setText(0, t.name + status);

                // Coloration orange pour les tests interrompus (on peut voir graphiquement les tests qui ont réussis ou non)
                if (t.endedAt.startsWith("INTERRUPTED:"))
                    item->setForeground(0, QColor("#e3671b"));

                QString tip = t.startedAt + " → " + tipEnd;
                item->setToolTip(0, tip);
            }
        });
    }

    testBox->setLayout(testLayout);
    testPageLayout->addWidget(testBox);

    modeStack->addWidget(testPage);


    //------------------------------------------------------------------------
    // --- Sélection par plage horaire ---
    QWidget* timePage = new QWidget(extraction_tab);
    QVBoxLayout* timePageLayout = new QVBoxLayout(timePage);

    QGroupBox* timeBox = new QGroupBox("Plage horaire", timePage);
    QGridLayout* timeLayout = new QGridLayout(timeBox);

    // Début
    QLabel* startLabel = new QLabel("Début :", timeBox);
    QDateTimeEdit* startTime = new QDateTimeEdit(timeBox);
    startTime->setDisplayFormat("yyyy-MM-dd HH:mm:ss.zzz");
    startTime->setDateTime(QDateTime(QDate(2026, 6, 1), QTime(0, 0, 0, 0)));
    startTime->setCalendarPopup(true);

    // Fin
    QLabel* endLabel = new QLabel("Fin :", timeBox);
    QDateTimeEdit* endTime = new QDateTimeEdit(timeBox);
    endTime->setDisplayFormat("yyyy-MM-dd HH:mm:ss.zzz");
    endTime->setDateTime(QDateTime(QDate(2026, 6, 1), QTime(0, 0, 0, 0)));
    endTime->setCalendarPopup(true);

    timeLayout->addWidget(startLabel, 0, 0);
    timeLayout->addWidget(startTime, 0, 1);
    timeLayout->addWidget(endLabel, 1, 0);
    timeLayout->addWidget(endTime, 1, 1);

    timeBox->setLayout(timeLayout);
    timePageLayout->addWidget(timeBox);

    modeStack->addWidget(timePage);
    mainLayout->addWidget(modeStack);

    //-------------------------------------------------------------------------
    // Connexion des modes
    QObject::connect(modeTest, &QRadioButton::toggled, extraction_tab, [=](bool checked){
        if (checked)
            modeStack->setCurrentIndex(0); // page Test
    });

    QObject::connect(modeTime, &QRadioButton::toggled, extraction_tab, [=](bool checked){
        if (checked)
            modeStack->setCurrentIndex(1); // page Plage horaire
    });

    //---------------------------------------------------------------------------
    // --- Boutons de fin ---
    QHBoxLayout* actionLayout = new QHBoxLayout();

    QPushButton* validateBtn = new QPushButton("Valider la sélection", extraction_tab);
    QPushButton* exportBtn   = new QPushButton("Exporter CSV", extraction_tab);
    QPushButton* purgeBtn    = new QPushButton("Purger données idle", extraction_tab);

    validateBtn->setFixedWidth(scale_x(200));
    exportBtn->setFixedWidth(scale_x(200));
    purgeBtn->setFixedWidth(scale_x(200));

    actionLayout->addWidget(validateBtn);
    actionLayout->addWidget(exportBtn);
    actionLayout->addWidget(purgeBtn);


    validateBtn->setStyleSheet(R"(
    QPushButton {
        background-color: #2ecc71;
        color: white;
        border: 1px solid #27ae60;
        border-radius: 6px;
        padding: 6px 12px;
        font-weight: bold;
    }
    QPushButton:hover {
        background-color: #27ae60;
    }
    QPushButton:pressed {
        background-color: #1e8449;
    }
    )");

    exportBtn->setStyleSheet(R"(
    QPushButton {
        background-color: #3498db;
        color: white;
        border: 1px solid #2980b9;
        border-radius: 6px;
        padding: 6px 12px;
        font-weight: bold;
    }
    QPushButton:hover {
        background-color: #2980b9;
    }
    QPushButton:pressed {
        background-color: #1f618d;
    }
    )");

    purgeBtn->setStyleSheet(R"(
    QPushButton {
        background-color: #e74c3c;
        color: white;
        border: 1px solid #c0392b;
        border-radius: 6px;
        padding: 6px 12px;
        font-weight: bold;
    }
    QPushButton:hover {
        background-color: #c0392b;
    }
    QPushButton:pressed {
        background-color: #922b21;
    }
    )"

    );

    //------------------------------------------------------------------------
    // Fonction : récupérer les capteurs sélectionnés
    auto getSelectedSensors = [=]() {
        QStringList selected;

        for (int i = 0; i < sensorTree->topLevelItemCount(); ++i) {
            QTreeWidgetItem* category = sensorTree->topLevelItem(i);

            for (int j = 0; j < category->childCount(); ++j) {
                QTreeWidgetItem* item = category->child(j);

                if (item->checkState(0) == Qt::Checked)
                    selected << item->text(0);
            }
        }
        return selected;
    };

    //------------------------------------------------------------------------------
    // Fonction : récupérer les IDs des tests sélectionnés (depuis Qt::UserRole)
    auto getSelectedTestIds = [=]() {
        QList<int> selected;
        if (!testList) return selected;
        for (int i = 0; i < testList->topLevelItemCount(); ++i) {
            QTreeWidgetItem* item = testList->topLevelItem(i);
            if (!item) continue;
            if (item->checkState(0) == Qt::Checked)
                selected << item->data(0, Qt::UserRole).toInt();
        }
        return selected;
    };

    //------------------------------------------------------------------------------
    // Mapping nom UI → nom colonne DB (pour FS01 et CM81 qui sont diffèrent, honnêtement ce n'était pas obligé)
    auto toDbColumn = [](const QString& uiName) -> QString {
        if (uiName == "FS01") return "LC";
        if (uiName == "CM81") return "glowplug";
        return uiName;  // PS*, TS*, FM* sont identiques
    };

    //------------------------------------------------------------------------
    // Bouton Valider : affiche un résumé de la sélection dans une boîte de dialogue
    QObject::connect(validateBtn, &QPushButton::clicked, extraction_tab, [=]() {
        QStringList sensors = getSelectedSensors();
        QString summary;

        if (modeTest->isChecked()) {
            QStringList testLabels;
            for (int i = 0; i < testList->topLevelItemCount(); ++i) {
                QTreeWidgetItem* item = testList->topLevelItem(i);
                if (item && item->checkState(0) == Qt::Checked) {
                    int id = item->data(0, Qt::UserRole).toInt();
                    QString startedAt = item->data(0, Qt::UserRole + 1).toString();
                    testLabels << QString("%1 (%2)").arg(id).arg(startedAt);
                }
            }
            summary = QString("Mode : Test\nTests sélectionnés : %1\nCapteurs : %2")
                          .arg(testLabels.isEmpty() ? "aucun" : testLabels.join(", "))
                          .arg(sensors.isEmpty() ? "aucun" : sensors.join(", "));
        } else {
            summary = QString("Mode : Plage horaire\nDébut : %1\nFin : %2\nCapteurs : %3")
                          .arg(startTime->dateTime().toString("yyyy-MM-dd HH:mm:ss.zzz"))
                          .arg(endTime->dateTime().toString("yyyy-MM-dd HH:mm:ss.zzz"))
                          .arg(sensors.isEmpty() ? "aucun" : sensors.join(", "));
        }

        QMessageBox::information(extraction_tab, "Résumé de la sélection", summary);
    });

    //------------------------------------------------------------------------
    // Bouton Export CSV : appelle DatabaseManager::exportToCsv()
    QObject::connect(exportBtn, &QPushButton::clicked, extraction_tab, [=]() {
        // 1) Base de données disponible ?
        if (!dbManager) {
            QMessageBox::warning(extraction_tab, "Export CSV", "Base de données non disponible.");
            return;
        }

        // 2) Vérifications selon le mode (avant d'ouvrir le sélecteur de fichier)
        QList<int> ids;
        QDateTime from, to;

        if (modeTest->isChecked()) {
            ids = getSelectedTestIds();
            if (ids.isEmpty()) {
                QMessageBox::warning(extraction_tab, "Export CSV", "Aucun test sélectionné.");
                return;
            }
        } else {
            from = startTime->dateTime();
            to   = endTime->dateTime();
            if (!from.isValid() || !to.isValid() || from >= to) {
                QMessageBox::warning(extraction_tab, "Export CSV",
                    "Plage horaire invalide : la date de début doit être antérieure à la date de fin.");
                return;
            }
        }

        // 3) Vérifier qu'il y a des données à exporter
        int rowCount = 0;
        if (modeTest->isChecked()) {
            for (int testId : ids)
                rowCount += dbManager->countData(testId);
        } else {
            rowCount = dbManager->countData(-1, from, to);
        }
        if (rowCount == 0) {
            QMessageBox::warning(extraction_tab, "Export CSV",
                "Aucune donnée disponible pour la sélection.\n"
                "Le CSV ne contiendrait que les en-têtes et n'a pas été créé.");
            return;
        }

        // 4) Convertit les noms UI → noms colonnes DB
        QStringList uiSensors  = getSelectedSensors();
        QStringList dbSensors;
        for (const QString& name : uiSensors)
            dbSensors << toDbColumn(name);

        // 5) Demande le chemin de sortie (seulement si tout est valide)
        QString outputPath = QFileDialog::getSaveFileName(
            extraction_tab, "Enregistrer le CSV",
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/velaryon_export.csv",
            "CSV (*.csv)"
        );
        if (outputPath.isEmpty()) return;

        // 6) Export
        bool ok = false;

        if (modeTest->isChecked()) {
            // Si plusieurs tests cochés : exporte chacun dans un fichier séparé (suffixe _testID)
            for (int testId : ids) {
                QString path = ids.size() == 1
                    ? outputPath
                    : outputPath.chopped(4) + QString("_test%1.csv").arg(testId);
                ok = dbManager->exportToCsv(path, dbSensors, testId);
            }
        } else {
            ok = dbManager->exportToCsv(outputPath, dbSensors, -1, from, to);
        }

        if (ok)
            QMessageBox::information(extraction_tab, "Export CSV", "CSV exporté avec succès !");
        else
            QMessageBox::critical(extraction_tab, "Export CSV", "Erreur lors de l'export. Vérifiez les logs.");
    });

    //------------------------------------------------------------------------
    // Bouton Purger : supprime les données idle (is_test = 0) de la DB
    QObject::connect(purgeBtn, &QPushButton::clicked, extraction_tab, [=]() {
        if (!dbManager) {
            QMessageBox::warning(extraction_tab, "Purge", "Base de données non disponible.");
            return;
        }
        auto reply = QMessageBox::question(extraction_tab, "Confirmer la purge",
            "Supprimer toutes les données collectées HORS test (idle) ?\n"
            "Les données des tests resteront intactes.",
            QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            dbManager->clearIdleData();
            QMessageBox::information(extraction_tab, "Purge", "Données idle supprimées.");
        }
    });

    //-----------------------------------------------------------------------
    // --- Récupération des dates choisis dans une liste ---
    QDateTime debut = startTime->dateTime();
    QDateTime fin   = endTime->dateTime();


    mainLayout->addLayout(topBar);           // logo + exit
    mainLayout->addWidget(sensorBox);       // 1) Capteurs
    mainLayout->addWidget(modeBox);        // 2) Mode de sélection
    mainLayout->addWidget(modeStack);     // 3) Test / Plage horaire
    mainLayout->addLayout(actionLayout); // 4) Boutons

    extraction_tab->setLayout(mainLayout);

    return extraction_tab;
}

//---------------------------------------------------------------------------
// --- Fonction Pour activer tous les capteurs ---
void setAllSensorsState(QTreeWidget* tree, Qt::CheckState state)
{
    if (!tree)
        return;

    for (int i = 0; i < tree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* category = tree->topLevelItem(i);

        for (int j = 0; j < category->childCount(); ++j) {
            category->child(j)->setCheckState(0, state);
        }
    }
}

