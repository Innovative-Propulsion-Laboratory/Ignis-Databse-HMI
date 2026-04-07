
/******************************************************************************************
**                                                                                       **
**   Graphic generates the main content for the "Graphic" tab in the user interface.     **
**   This tab includes 4 plot sections, each of which can be enabled or disabled at      **
**   compile time. Each section features a real-time graph display and a control panel   **
**   that allows adjusting the time range (e.g., last 1 min, last 10 min) and selecting  **
**    which curves to display.                                                           **
**                                                                                       **
*******************************************************************************************
**                                                                                       **
**   This part of the code could be significantly optimized for better readability       **
**   and efficiency. Although the maximum curve update frequency is 500 Hz, the          **
**   actual limit is likely imposed by the UdpReceiver. Both components will probably    **
**   require some refactoring to improve performance.                                    **
**                                                                                       **
******************************************************************************************/


#include <QWidget>
#include <QFrame>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QGroupBox>
#include <QMap>
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include "Graphic/qcustomplot.h"
#include "UDP/udpreceiver.h"
#include <QDebug>
#include <tools/closeButton.h>
#include "tools/scaler.h"

// Selected curve IDs to display
QVector<int> selectedIDs = {0,12,18,21};

// X axis time range for each plot (in seconds)
QVector<double> selectedTimeRange = {10.0,10.0,10.0,10.0};

// Curve names
QVector<QString> names = {  "PS11", "PS12", "PS21", "PS22", "PS31", "PS41", "PS42", "PS51", "PS61", "PS62", "PS63", "PS64", "PS81",
                            "TS11", "TS12", "TS41", "TS42", "TS61", "TS62",
                            "FM11", "FM21", "FM61",
                            "FS01" };

// Stores name and color of curves (improvement can be made on color choice)
QMap<QString, QString> checkboxColors = {
    { "PS11", "#0000FF" }, { "PS12", "#388AFF" }, { "PS21", "#FF0000" }, { "PS22", "#FF6363" }, { "PS31", "#08CC0A" }, { "PS41", "#B26701" },
    { "PS42", "#DB7F01" }, { "PS51", "#B700FF" }, { "PS61", "#FF9C00" }, { "PS62", "#F08B08" }, { "PS63", "#FFDD00" }, { "PS64", "#FFBB2A" }, {"PS81", "#FFBB7A"},
    { "TS11", "#0000FF" }, { "TS12", "#60A917" }, { "TS41", "#B26701" }, { "TS42", "#DB7F01" }, { "TS61", "#FF9C00" }, { "TS62", "#F08B08" },
    { "FM11", "#0000FF" }, { "FM21", "#FF0000" }, { "FM61", "#FF9C00" },
    { "FS01", "#B26701" }
};

// Stores Y lower/max values
QVector<QVector<double>> bounds(24, QVector<double>{-1, 10});

// Original realtimeDataSlot kept for reference — NOT used at runtime anymore.
// The optimized path uses addDataToPlot() + a 30 Hz QTimer for replot().
void realtimeDataSlot(QCustomPlot* customPlot, const QVector<QVariant>& sensorData, const QVector<int>& selectedIDs, int graphType)
{
    // Number of curves per sensor
    int curveCount, j = 0;
    switch (graphType) {
    case 0: curveCount = 13; j = 0; break;   // Pressure
    case 1: curveCount = 6;  j = 12; break;  // Temperature
    case 2: curveCount = 3;  j = 18; break;  // Flow
    case 3: curveCount = 1;  j = 21; break;  // Force
    default: curveCount = 0; break;
    }

    double time = sensorData[1].toDouble(); // Time in second

    double minY = std::numeric_limits<double>::max();
    double maxY = std::numeric_limits<double>::lowest();

    // Loop through curves for plotting
    for (int i = 0; i < curveCount; i++)
    {
        int curveID = i + j;

        // Convert data into double
        double value = 0;
        if (sensorData[curveID + 2].canConvert<uint32_t>()) {
            value = static_cast<double>(sensorData[curveID + 2].toDouble());
        } else if (sensorData[curveID + 2].canConvert<uint16_t>()) {
            value = static_cast<double>(sensorData[curveID + 2].toDouble());
        } else if (sensorData[curveID + 2].canConvert<uint8_t>()) {
            value = static_cast<double>(sensorData[curveID + 2].toDouble());
        } else {
            qDebug() << "Unsupported data type!";
            continue;
        }

        // Add new data to curve
        customPlot->graph(i)->addData(time, value);

        // Update the stored Y bounds for this curve
        if (value < bounds[curveID][0]) bounds[curveID][0] = value; // lower
        if (value > bounds[curveID][1]) bounds[curveID][1] = value; // upper

        // If this curve is selected, update the overall plot Y bounds too
        if (selectedIDs.contains(curveID)) {
            minY = std::min(minY, bounds[curveID][0]);
            maxY = std::max(maxY, bounds[curveID][1]);
        }

        // Plot curves if selected
        QPen pen;
        if (selectedIDs.contains(curveID)) {
            pen.setColor(checkboxColors[names[curveID]]);
            pen.setWidth(3);
        } else {
            pen.setColor(QColor(0,0,0,0));  // transparent
            pen.setWidth(1);
        }
        customPlot->graph(i)->setPen(pen);
    }

    // Apply margin for readability
    if (minY<-50) minY = 1.1*minY;
    if (maxY>50) maxY = 1.1*maxY;
    if (minY < maxY) {
        customPlot->yAxis->setRange(minY-1, maxY+1);
    }

    // Shift X range to show last N seconds
    customPlot->xAxis->setRange(time, selectedTimeRange[graphType], Qt::AlignRight);
    customPlot->replot();
}

// ---- OPTIMISATION : ajoute les donnees sans replot() ----
// Le replot() est gere par un QTimer dedie a 30 Hz (voir connexion plus bas).
// Cela evite 400+ replot()/s a haute frequence UDP.
void addDataToPlot(QCustomPlot* customPlot, const QVector<QVariant>& sensorData, const QVector<int>& selectedIDs, int graphType)
{
    int curveCount, j = 0;
    switch (graphType) {
    case 0: curveCount = 13; j = 0;  break;
    case 1: curveCount = 6;  j = 13; break;
    case 2: curveCount = 3;  j = 19; break;
    case 3: curveCount = 1;  j = 22; break;
    default: curveCount = 0; break;
    }

    double time = sensorData[1].toDouble();

    for (int i = 0; i < curveCount; i++)
    {
        int curveID = i + j;

        double value = 0;
        if (sensorData[curveID + 2].canConvert<uint32_t>()) {
            value = static_cast<double>(sensorData[curveID + 2].toDouble());
        } else if (sensorData[curveID + 2].canConvert<uint16_t>()) {
            value = static_cast<double>(sensorData[curveID + 2].toDouble());
        } else if (sensorData[curveID + 2].canConvert<uint8_t>()) {
            value = static_cast<double>(sensorData[curveID + 2].toDouble());
        } else {
            qDebug() << "Unsupported data type!";
            continue;
        }

        // Ajoute le point sans redessiner
        customPlot->graph(i)->addData(time, value);

        // Met a jour les bornes Y
        if (value < bounds[curveID][0]) bounds[curveID][0] = value;
        if (value > bounds[curveID][1]) bounds[curveID][1] = value;

        // Pen color
        QPen pen;
        if (selectedIDs.contains(curveID)) {
            pen.setColor(checkboxColors[names[curveID]]);
            pen.setWidth(3);
        } else {
            pen.setColor(QColor(0,0,0,0));
            pen.setWidth(1);
        }
        customPlot->graph(i)->setPen(pen);
    }
}

// Create curve slots
void setupRealtimePlot(QCustomPlot* customPlot, int graphType)
{
    // Define number of curves per plot section
    int curveCount; // curveCount must be changed manually for a different number of sensor
    switch (graphType) {
    case 0: curveCount = 13; break;
    case 1: curveCount = 6;  break;
    case 2: curveCount = 3;  break;
    case 3: curveCount = 1;  break;
    default: curveCount = 0; break;
    }

    // Create N curves depending on the number of sensors
    for (int i = 0; i < curveCount; ++i)
    {
        customPlot->addGraph();
    }

    // X-axis displayed in hour format
    QSharedPointer<QCPAxisTickerTime> timeTicker(new QCPAxisTickerTime);
    timeTicker->setTimeFormat("%h:%m:%s");
    customPlot->xAxis->setTicker(timeTicker);

    // Ensure correct display of the axis
    customPlot->axisRect()->setupFullAxesBox();

    // Initial Y-axis
    customPlot->yAxis->setRange(-100, 100);
}

QWidget* Graphic(QWidget* parent,UdpReceiver& receiver)
{
    // Create graph tab
    QWidget* graphic_tab = new QWidget(parent);
    QHBoxLayout* mainLayout = new QHBoxLayout(graphic_tab);

    // Create plot frame & vertical layout
    QFrame* plot_frame = new QFrame(graphic_tab);
    plot_frame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QVBoxLayout* plot_layout = new QVBoxLayout(plot_frame);
    plot_frame->setLayout(plot_layout);

    // Create checkbox frame & vertical layout
    QFrame* checkbox_frame = new QFrame(graphic_tab);
    checkbox_frame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QVBoxLayout* checkbox_layout = new QVBoxLayout(checkbox_frame);
    checkbox_frame->setLayout(checkbox_layout);

    // Plot frame takes 90% of the window and checkbox frame 10%
    mainLayout->addWidget(plot_frame, 9);
    mainLayout->addWidget(checkbox_frame, 1);

    QVector<QCustomPlot*> plotWidgets;

    // Instantiate 4 QCustomPlot widgets (pressure, temperature, flow rate and force)
    for (int i = 0; i < 4; ++i)
    {
        QCustomPlot* plot = new QCustomPlot();
        setupRealtimePlot(plot,i);
        plotWidgets.append(plot);
    }

    // Selected curve section to display ( 1 = show, 0 = hide)
    QVector<int> curveSection(plotWidgets.size(), 1);

    // The following loop could be optimize for better versatility (use of struct, encapsulation,...)
    // Grouping the section (pressure, temperature,...)
    QMap<QString, QGroupBox*> categoryGroups;

    // Grouping the checkboxes
    QMap<QString, QGridLayout*> categoryLayouts;

    // Match abbreviation with section name
    QMap<QString, QString> categoryMapping = {
        { "PS", "Pressure section" },
        { "TS", "Temperature section" },
        { "FM", "Flowrate section" },
        { "FS", "Force section" }
    };

    QStringList categories = { "Pressure section", "Temperature section", "Flowrate section", "Force section" };

    for (int i = 0; i < curveSection.size(); i++) {
        if (curveSection[i] == 1) {

            // Add plot section
            plot_layout->addWidget(plotWidgets[i]);

            QGroupBox* groupBox = new QGroupBox(categories[i]);

            // Set fixed size and location for the group box
            groupBox->setFixedSize(scale_x(200), scale_x(180));
            groupBox->move(0, 0);

            QVBoxLayout* groupBoxLayout = new QVBoxLayout(groupBox);  // Main layout for the groupBox
            QGridLayout* buttonLayout = new QGridLayout();

            // Add time range buttons
            QStringList timeLabels = { "1m", "10m", "30m", "All" };
            // Add time range buttons
            QMap<QString, double> timeRanges = {
                { "1m", 60 },
                { "10m", 600 },
                { "30m", 1800 },
                { "All", 3600 }
            };

            int row = 0, col = 0;
            for (const QString& label : timeLabels) {
                QPushButton* button = new QPushButton(label);

                // Apply custom stylesheet to the button
                button->setStyleSheet(R"(
                    QPushButton {
                        border: 1px solid black;
                        background-color: white;
                        border-radius: 3px;
                        height: 20px;
                    }
                    QPushButton:hover {
                        background-color: darkgray;
                        border-color: darkgray;
                    }
                )");

                // Add the button to the grid layout
                buttonLayout->addWidget(button, row, col);
                col++;
                if (col == 2) {
                    col = 0;
                    row++;
                }
                // *Connect button to adjust x-axis range**
                QObject::connect(button, &QPushButton::clicked, [=]() {
                    selectedTimeRange[i] = timeRanges[label];  // Assign the value to the correct index

                    // Iterate over all widgets in the buttonLayout and reset them
                    for (int r = 0; r < buttonLayout->rowCount(); ++r) {
                        for (int c = 0; c < buttonLayout->columnCount(); ++c) {
                            QLayoutItem* item = buttonLayout->itemAtPosition(r, c);
                            if (item) {
                                QPushButton* btn = qobject_cast<QPushButton*>(item->widget());
                                if (btn) {
                                    btn->setStyleSheet(R"(
                                        QPushButton {
                                            border: 1px solid black;
                                            background-color: white;
                                            border-radius: 3px;
                                            height: 20px;
                                        }
                                        QPushButton:hover {
                                            background-color: darkgray;
                                            border-color: darkgray;
                                        }
                                    )");
                                }
                            }
                        }
                    }

                    // Change the clicked button to black
                    button->setStyleSheet(R"(
                        QPushButton {
                            border: 1px solid black;
                            background-color: lightgray;
                            border-radius: 3px;
                            height: 20px;
                        }
                        QPushButton:hover {
                            background-color: darkgray;
                            border-color: darkgray;
                        }
                    )");
                });

            }

            // Add button layout to the groupBox
            groupBoxLayout->addLayout(buttonLayout);

            // Create a QGridLayout for checkboxes (for multi-column layout with 4 rows max)
            QGridLayout* checkboxLayoutInGroup = new QGridLayout();
            groupBoxLayout->addLayout(checkboxLayoutInGroup);  // Add checkbox layout to the group box

            categoryLayouts[categories[i]] = checkboxLayoutInGroup;  // Store the checkbox layout for later
            categoryGroups[categories[i]] = groupBox;

            // Add category groups to the checkbox layout
            checkbox_layout->addWidget(groupBox);
        }
    }

    // Adding checkboxes to the corresponding category group layout
    QMap<QString, int> rowCount;    // Track rows for each category
    int maxRows = 4;                // Limit rows before creating a new column
    int idCounter = 0;

    for (const QString& name : names) {
        QString category = name.left(2);  // Extracts "PS", "TS", "FM", or "FS"
        if (categoryMapping.contains(category)) {
            QString fullCategory = categoryMapping[category];
            if (categoryLayouts.contains(fullCategory)) {
                int row = rowCount[fullCategory] % maxRows;
                int col = rowCount[fullCategory] / maxRows;  // Create new column after maxRows

                // Create checkbox and style it
                QCheckBox* checkbox = new QCheckBox(name);
                checkbox->setStyleSheet(
                    "QCheckBox::indicator {"
                    "    width: 14px;"
                    "    height: 14px;"
                    "    border: 1px solid lightgray;"
                    "    border-radius: 3px;"
                    "}"
                    );

                // Default color
                QString color = checkboxColors.value(name, "#FFFFFF");

                // Initial curves to plot
                if (selectedIDs.contains(names.indexOf(name)))
                {
                    checkbox->setChecked(true);
                    checkbox->setStyleSheet(QString(
                                                "QCheckBox::indicator {"
                                                "    width: 14px;"  // Set a fixed width
                                                "    height: 14px;" // Set a fixed height
                                                "    background-color: %1;"
                                                "    border-radius: 3px;"
                                                "}"
                                                ).arg(color));

                }

                // Connect checkbox to toggle for curve selection
                QObject::connect(checkbox, &QCheckBox::toggled, [=](bool checked) {
                    if (checked) {
                        if (!selectedIDs.contains(idCounter)) {
                            selectedIDs.append(idCounter);
                        }
                        checkbox->setStyleSheet(QString(
                                                    "QCheckBox::indicator {"
                                                    "    width: 14px;"  // Set a fixed width
                                                    "    height: 14px;" // Set a fixed height
                                                    "    background-color: %1;"
                                                    "    border-radius: 3px;"
                                                    "}"
                                                    ).arg(color));

                    } else {
                        selectedIDs.removeAll(idCounter);
                        checkbox->setStyleSheet(
                            "QCheckBox::indicator {"
                            "    width: 14px;"
                            "    height: 14px;"
                            "    border: 1px solid lightgray;"
                            "    border-radius: 3px;"
                            "}"
                            );
                    }
                });
                idCounter++;
                categoryLayouts[fullCategory]->addWidget(checkbox, row, col);   // Add checkbox to QGridLayout at (row, col)
                rowCount[fullCategory]++;
            }
        }
    }

    // ---- OPTIMISATION : decouplage donnees / replot ----
    // Le signal sensor_value appelle addDataToPlot() (sans replot).
    // Un QTimer dedie a ~30 Hz appelle replot() + mise a jour des axes.
    // Resultat : meme a 500 Hz UDP, on ne fait que 4x30 = 120 replot/s au lieu de 4x500 = 2000.

    // Variable partagee pour stocker le dernier temps recu par plot.
    // Allouee via QSharedPointer pour survivre apres le retour de Graphic().
    auto lastTime = QSharedPointer<QVector<double>>::create(QVector<double>(4, 0.0));

    int graph_id = 0;
    for (QCustomPlot* plot : plotWidgets)
    {
        int gid = graph_id;  // capture locale

        // 1) Le signal ajoute les donnees sans replot
        QObject::connect(&receiver, &UdpReceiver::sensor_value, plot, [=](const QVector<QVariant>& data) {
            addDataToPlot(plot, data, selectedIDs, gid);
            (*lastTime)[gid] = data[1].toDouble();  // stocke le dernier temps
        }, Qt::QueuedConnection);

        // 2) Timer dedie : replot a ~30 Hz
        QTimer* replotTimer = new QTimer(plot);
        replotTimer->setInterval(33);  // ~30 Hz
        QObject::connect(replotTimer, &QTimer::timeout, plot, [=]() {
            double t = (*lastTime)[gid];
            if (t <= 0.0) return;  // pas encore de donnees

            // Mise a jour des axes Y (bornes des courbes selectionnees)
            int curveCount, j_offset = 0;
            switch (gid) {
            case 0: curveCount = 13; j_offset = 0;  break;
            case 1: curveCount = 6;  j_offset = 13; break;
            case 2: curveCount = 3;  j_offset = 19; break;
            case 3: curveCount = 1;  j_offset = 22; break;
            default: curveCount = 0; break;
            }

            double minY = std::numeric_limits<double>::max();
            double maxY = std::numeric_limits<double>::lowest();
            for (int i = 0; i < curveCount; i++) {
                int curveID = i + j_offset;
                if (selectedIDs.contains(curveID)) {
                    minY = std::min(minY, bounds[curveID][0]);
                    maxY = std::max(maxY, bounds[curveID][1]);
                }
            }

            if (minY < -50) minY = 1.1 * minY;
            if (maxY > 50)  maxY = 1.1 * maxY;
            if (minY < maxY) {
                plot->yAxis->setRange(minY - 1, maxY + 1);
            }

            plot->xAxis->setRange(t, selectedTimeRange[gid], Qt::AlignRight);
            plot->replot();
        });
        replotTimer->start();

        graph_id++;
    }

    // Ensure all checkbox layouts have their 16 cells occupied to maintain alignment
    for (auto& category : categoryLayouts.keys()) {
        int totalItems = rowCount[category];
        int placeholderCount = qMax(0, 16 - totalItems);

        // Add placeholder labels for remaining spaces
        for (int i = 0; i < placeholderCount; ++i) {
            QLabel* placeholder = new QLabel("");   // Empty QLabel as a spacer
            placeholder->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);     // Prevent overlap

            int row = (totalItems + i) % maxRows;
            int col = (totalItems + i) / maxRows;

            categoryLayouts[category]->addWidget(placeholder, row, col);
        }
    }

    graphic_tab->setLayout(mainLayout);

    return graphic_tab;
}
