/******************************************************************************************
**                                                                                       **
**   TestTab generates the main content for the "Test" tab in the user interface.        **
**   This tab includes several control frames such as tank pressure control, TVC,        **
**   sequence selection, emergency control and logs.                                     **
**   Any test (cold flows, hotfires,...) will be done mostly through this tab.           **
**                                                                                       **
******************************************************************************************/
#include <QWidget>
#include <QTabWidget>
#include <QFont>
#include <QFrame>
#include <QVBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QDoubleSpinBox>
#include <QRadioButton>
#include <QSlider>
#include <QPushButton>
#include <Qcombobox>
#include <QTimer>
#include <QPlainTextEdit>
#include "Graphic/qcustomplot.h"
#include "UDP/udpsender.h"
#include "UDP/udpreceiver.h"
#include "UDP/global_variable.h"
#include "tools/loghelper.h"
#include "tools/scaler.h"
#include <QFileDialog>
#include <QFileInfo>


// Initialize the log terminal
QPlainTextEdit* globalLogTerminal = nullptr;

QList<int> valuesList(30, 999);     // Array of int with default 999
QStringList sequence_registering;   // Holds selected sequence content

// Update combobox to show sequence files
void update_combobox(QComboBox* comboBox) {
    QDir dir(":resources/Sequence/");

    // Get a list of all .txt files in the directory
    QStringList sequence_file = dir.entryList(QStringList() << "*.txt", QDir::Files);

    comboBox->clear();

    // If no .txt files are found, inform the user
    if (sequence_file.isEmpty()) {
        comboBox->addItem("No .txt files found");
    }

    // Else, add all found .txt files to the combo box
    else {
        comboBox->addItem("None");
        comboBox->addItems(sequence_file);
    }
}

void update_labels_from_file(QGridLayout* labels_grid, const QString& filename) {
    QLayoutItem* item;
    while ((item = labels_grid->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }

    if (filename.isEmpty()) {
        valuesList.clear();
        for (int row = 0; row < 6; ++row) {
            for (int col = 0; col < 5; ++col) {
                QLabel* empty_label = new QLabel("---:---");
                empty_label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
                empty_label->setContentsMargins(30, 0, 0, 0);
                labels_grid->addWidget(empty_label, row, col);
                valuesList.append(999);
            }
        }
        labels_grid->update();
        return;
    }

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(nullptr, "File Error", "Cannot open file: " + filename);
        return;
    }

    QTextStream in(&file);
    QStringList lines;
    while (!in.atEnd()) {
        lines.append(in.readLine());
    }
    file.close();

    int row = 0, col = 0;
    valuesList.clear();
    sequence_registering.clear();

    for (const QString& line : lines) {
        sequence_registering.append(line);
        QStringList parts = line.split(",");

        if (parts.size() >= 2) {
            QString title = parts[0].trimmed();
            QString valueStr = parts[1].trimmed();
            QString unit = (parts.size() >= 3) ? parts[2].trimmed() : " ";

            bool ok;
            int value = valueStr.toInt(&ok);
            if (ok) valuesList.append(value);

            QString formattedText = QString("%1: %2 %3").arg(title).arg(valueStr).arg(unit);

            QLabel* label = new QLabel(formattedText);
            label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            label->setContentsMargins(30, 0, 0, 0);
            labels_grid->addWidget(label, row, col);

            if (++row == 6) {
                row = 0;
                if (++col == 5) break;
            }
        }
    }

    labels_grid->update();
}
// Call sequence display function when sequence selected
void on_combobox_selection_changed(QComboBox* comboBox, QGridLayout* labels_grid) {
    QString selected_file = comboBox->currentText();
    if (selected_file == "No .txt files found" || selected_file.isEmpty())
        return;

    update_labels_from_file(labels_grid, selected_file);
}


void select_sequence_file_and_update(QGridLayout* labels_grid, QWidget* parent = nullptr) {
    QString filePath = QFileDialog::getOpenFileName(
        parent,
        "Select a sequence file",
        QDir::homePath(),                 // dossier de départ (tu peux mettre autre chose)
        "Sequence files (*.txt);;All files (*.*)"
        );

    if (filePath.isEmpty())
        return;

    update_labels_from_file(labels_grid, filePath);
}


QWidget* TestTab(QWidget* parent, QTabWidget* tabwidget, UdpReceiver& receiver, DatabaseManager* dbManager)
{
    //------------------------------------------------------------------------
    // Create test tab and retrieve existing tabs
    QWidget* engine_tab = tabwidget->widget(0);
    QWidget* cooling_tab = tabwidget->widget(1);
    QWidget* test_tab = new QWidget(parent);

    QFont font = QFont("Arial", 10, QFont::Bold);
    QFont font2 = QFont("Arial", 10, QFont::Bold);

    //------------------------------------------------------------------------
    // Pressure control frame
    QFrame* pressurisation_frame = new QFrame(test_tab);
    pressurisation_frame->setFrameShape(QFrame::StyledPanel);
    pressurisation_frame->setGeometry(QRect(scale_x(10), scale_x(10), scale_x(400), scale_x(341)));

    QVBoxLayout* layout_pressurisation_frame = new QVBoxLayout(pressurisation_frame);
    layout_pressurisation_frame->setAlignment(Qt::AlignCenter);

    // Title label for the pressure control section
    QLabel* pressurisation_title = new QLabel("Tank pressure control", pressurisation_frame);
    pressurisation_title->setFont(font);
    pressurisation_title->setAlignment(Qt::AlignCenter);
    layout_pressurisation_frame->addWidget(pressurisation_title);

    // Spacer label to add space below the title
    layout_pressurisation_frame->addWidget(new QLabel("  ", pressurisation_frame), 0, Qt::AlignCenter);

    // Layout for holding the individual tank controls
    QHBoxLayout* pressurisation_layout = new QHBoxLayout();
    pressurisation_layout->setAlignment(Qt::AlignCenter);

    // Define names, styles, and containers
    QVector<QDoubleSpinBox*> pressurisation_spinbox;                                    // Spin boxes for each tank
    QStringList spinbox_names = {"LOX pressure", "ETH pressure", "H2O pressure"};       // Tank names
    QVector<QLabel*> value_labels;                                                      // Labels that show current pressure
    QVector<QString> color_styles = {
        R"( QLabel { background-color: #1BA1E2; padding: 10px; border: 1px solid #0F7EBE; border-radius: 25px; } )",
        R"( QLabel { background-color: #E51400; padding: 10px; border: 1px solid #BF2322; border-radius: 25px; } )",
        R"( QLabel { background-color: #F0A30A; padding: 10px; border: 1px solid #BD7203; border-radius: 25px; } )"
    };

    // Create widgets for each tank control
    for (int i = 0; i < spinbox_names.size(); ++i) {
        QVBoxLayout* tank_control = new QVBoxLayout();
        tank_control->setAlignment(Qt::AlignCenter);

        // Tank label
        QLabel* label = new QLabel(spinbox_names[i], pressurisation_frame);
        label->setFont(font2);
        tank_control->addWidget(label, 0, Qt::AlignCenter);

        // Spinbox to set the pressure
        QDoubleSpinBox* spinbox = new QDoubleSpinBox(pressurisation_frame);
        spinbox->setFixedSize(scale_x(100), scale_x(30));
        spinbox->setLocale(QLocale(QLocale::English, QLocale::UnitedStates));
        spinbox->setStyleSheet(
            R"(QDoubleSpinBox {
        background-color: white;  /* Dark gray background */
        color: black;             /* Light text for contrast */
        border: 2px solid #696969;  /* Subtle blue border */
        border-radius: 5px;
        padding: 5px;
        font-size: 14px;
        }

        QDoubleSpinBox::up-button {
            width: 30px;
            height: 15px;
            background-color: lightgray;
            border: none;
            border-radius: 2px;
            image: url(:/ressources/pictures/arrow_up.png); /* Custom up arrow icon */
        }

        QDoubleSpinBox::down-button {
            width: 30px;
            height: 15px;
            background-color: lightgray;
            border: none;
            border-radius: 2px;
            image: url(:/ressources/pictures/arrow_down.png); /* Custom down arrow icon */
        }
        )");

        // Range and default value per tank type
        spinbox->setRange(i == 2 ? 1 : 1, i == 2 ? 10 : 20);
        spinbox->setValue(i == 2 ? 4 : 16);

        pressurisation_spinbox.append(spinbox);
        tank_control->addWidget(spinbox, 0, Qt::AlignCenter);

        // Tank visual
        QLabel* colorLabel = new QLabel(pressurisation_frame);
        colorLabel->setStyleSheet(color_styles[i]);
        colorLabel->setFixedSize(scale_x(50), scale_x(115));
        tank_control->addWidget(colorLabel, 0, Qt::AlignCenter);

        // Selected pressure target shown in engine or cooling tab
        QLabel* value_label = new QLabel(QString("%1 bar").arg(i == 2 ? 4 : 16), (i < 2) ? engine_tab : cooling_tab);
        value_label->setFont(font2);
        value_label->setAlignment(Qt::AlignCenter);
        value_labels.append(value_label);

        if (i < 2) {
            value_label->setGeometry(QRect(scale_x(644), scale_x(312 + i * 68), scale_x(40), scale_x(30)));  // Position labels in engine_tab
        } else {
            value_label->setGeometry(QRect(scale_x(669), scale_x(349), scale_x(40), scale_x(30)));  // Position label in cooling_tab
        }

        // Button to send set pressure command
        QPushButton* pressurisation_button = new QPushButton("Set pressure", pressurisation_frame);
        pressurisation_button->setFont(font);
        pressurisation_button->setFixedSize(scale_x(95), scale_x(30));
        pressurisation_button->setStyleSheet(R"(
        QPushButton {
            border: 1px solid black;
            background-color: white;
            border-radius: 4px;
            height: 30px;
        }
        QPushButton:hover {
            background-color: #ADADAD;
            border: 2px rgb(255,200,200);
        }
        )");

        // Connect pressure setter button to UdpSender
        QObject::connect(pressurisation_button, &QPushButton::clicked, [=]() {
            UdpSender* sender = new UdpSender();
            int tank;

            switch (i)
            {
            case 0:
                tank = 1;   // ETH
                break;
            case 1:
                tank = 2;   // LOX
                break;
            case 2:
                tank = 6;   // H2O
                break;
            }

            sender->sendsetpressure(tank,spinbox->value());
            value_label->setText(QString("%1 bar").arg(spinbox->value()));  // Update target pressure in engine/cooling tabs

        });
        tank_control->addWidget(pressurisation_button, 0, Qt::AlignCenter);

        // Button for enabling/disabling bang-bang control
        QPushButton* BB_button = new QPushButton("BB disabled", pressurisation_frame);
        BB_button->setFont(font);
        BB_button->setFixedSize(scale_x(95), scale_x(25));
        BB_button->setStyleSheet(R"(
        QPushButton {
            border: 1px solid black;
            background-color: white;
            border-radius: 4px;
            height: 30px;
        }
        QPushButton:hover {
            background-color: #ADADAD;
            border: 2px rgb(255,200,200);
        }
        )");
        tank_control->addWidget(BB_button, 0, Qt::AlignCenter);

        // Connect BB state setter button to Udp Sender
        QObject::connect(BB_button, &QPushButton::clicked, [=]() {
            UdpSender* sender = new UdpSender();
            int tank;

            switch (i)
            {
            case 0:
                tank = 1;
                break;
            case 1:
                tank = 2;
                break;
            case 2:
                tank = 6;
                break;
            }

            if (BB_button->text() == "BB disabled" && sender->sendBBcontrol(tank,1))
            {
                BB_button->setText("BB enabled");
            }
            else if(BB_button->text() == "BB enabled" && sender->sendBBcontrol(tank,0))
            {
                BB_button->setText("BB disabled");
            }
        });
        pressurisation_layout->addLayout(tank_control);
    }
    QVector<QLabel*> dynamic_sensor_values;
    QVector<QPoint> dynamic_sensor_values_position = {
        {55, 185}, {180, 185}, {305, 185}
    };

    // Create N labels for pressure sensor display
    for (int i = 0; i < dynamic_sensor_values_position.size(); ++i) {
        QLabel* label = new QLabel(test_tab);                                   // Create a new label for each sensor value
        label->setGeometry(scale_x(dynamic_sensor_values_position[i].x()), scale_x(dynamic_sensor_values_position[i].y()),
                           scale_x(60), scale_x(31));  // Set position and size
        label->setFont(QFont("Arial",10,QFont::Bold));                          // Set font style
        label->setStyleSheet(R"(background-color: transparent)");               // Set background color
        label->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);               // Align text to the center
        dynamic_sensor_values.append(label);                                    // Add label to the list
    }

    // Update labels when new values are obtained by the UdpReceiver
    QObject::connect(&receiver, &UdpReceiver::sensor_value, cooling_tab, [dynamic_sensor_values](QVector<QVariant> values) {
        dynamic_sensor_values[0]->setText(QString::number(values[2].toDouble()) + " \nbar");
        dynamic_sensor_values[1]->setText(QString::number(values[4].toDouble()) + " \nbar");
        dynamic_sensor_values[2]->setText(QString::number((values[10].toDouble() + values[11].toDouble()) / 2.0) + " \nbar");   // Average of H2O tanks
    });

    layout_pressurisation_frame->addLayout(pressurisation_layout);

    //------------------------------------------------------------------------
    // Actuator control frame
    QFrame* actuator_frame = new QFrame(test_tab);
    actuator_frame->setFrameShape(QFrame::StyledPanel);
    actuator_frame->setGeometry(QRect(scale_x(411), scale_x(10), scale_x(200), scale_x(341)));

    QVBoxLayout* layout_actuator_frame = new QVBoxLayout(actuator_frame);
    layout_actuator_frame->setAlignment(Qt::AlignCenter);

    // Title label for actuator control section
    QLabel* actuator_title = new QLabel("Actuator control",actuator_frame);
    actuator_title->setFont(font);
    actuator_title->setAlignment(Qt::AlignCenter);
    layout_actuator_frame->addWidget(actuator_title);

    // Radiobuttons to switch input types (angle or length)
    QHBoxLayout* layout_radio_button = new QHBoxLayout();
    QRadioButton* radio_button1 = new QRadioButton("Length", actuator_frame);
    QRadioButton* radio_button2 = new QRadioButton("Angle", actuator_frame);
    radio_button1->setChecked(true);        // Default length
    layout_radio_button->addWidget(radio_button1);
    layout_radio_button->addWidget(radio_button2);
    layout_actuator_frame->addLayout(layout_radio_button);

    // Separate left-right control
    QFrame* separator = new QFrame(actuator_frame);
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    layout_actuator_frame->addWidget(separator);

    // Spinbox for command inputs
    QHBoxLayout* layout_actuator_spinbox = new QHBoxLayout();
    QDoubleSpinBox* spinbox_left = new QDoubleSpinBox(actuator_frame);
    QDoubleSpinBox* spinbox_right = new QDoubleSpinBox(actuator_frame);

    spinbox_left->setLocale(QLocale(QLocale::English, QLocale::UnitedStates));
    spinbox_right->setLocale(QLocale(QLocale::English, QLocale::UnitedStates));

    spinbox_left->setStyleSheet(
        R"(QDoubleSpinBox {
        background-color: white;  /* Dark gray background */
        color: black;             /* Light text for contrast */
        border: 2px solid #696969;  /* Subtle blue border */
        border-radius: 5px;
        padding: 5px;
        font-size: 14px;
    }

    QDoubleSpinBox::up-button {
        width: 30px;
        height: 15px;
        background-color: lightgray;
        border: none;
        border-radius: 2px;
        image: url(:/ressources/pictures/arrow_up.png); /* Custom up arrow icon */
    }

    QDoubleSpinBox::down-button {
        width: 30px;
        height: 15px;
        background-color: lightgray;
        border: none;
        border-radius: 2px;
        image: url(:/ressources/pictures/arrow_down.png); /* Custom down arrow icon */
    }
    )");

    spinbox_right->setStyleSheet(
        R"(QDoubleSpinBox {
        background-color: white;  /* Dark gray background */
        color: black;             /* Light text for contrast */
        border: 2px solid #696969;  /* Subtle blue border */
        border-radius: 5px;
        padding: 5px;
        font-size: 14px;
    }

    QDoubleSpinBox::up-button {
        width: 30px;
        height: 15px;
        background-color: lightgray;
        border: none;
        border-radius: 2px;
        image: url(:/ressources/pictures/arrow_up.png); /* Custom up arrow icon */
    }

    QDoubleSpinBox::down-button {
        width: 30px;
        height: 15px;
        background-color: lightgray;
        border: none;
        border-radius: 2px;
        image: url(:/ressources/pictures/arrow_down.png); /* Custom down arrow icon */
    }
    )");

    // Set range and size
    spinbox_left->setRange(0.0, 100.0);
    spinbox_right->setRange(0.0, 100.0);
    spinbox_left->setFixedSize(90, 30);
    spinbox_right->setFixedSize(90, 30);
    spinbox_left->setDecimals(1);
    spinbox_right->setDecimals(1);

    layout_actuator_spinbox->addWidget(spinbox_left);
    layout_actuator_spinbox->addWidget(spinbox_right);

    // Slider control (redundacy with spinbox)
    layout_actuator_frame->addLayout(layout_actuator_spinbox);
    QHBoxLayout* layout_actuator_slider = new QHBoxLayout();

    QSlider* slider_left = new QSlider(Qt::Vertical, actuator_frame);
    QSlider* slider_right = new QSlider(Qt::Vertical, actuator_frame);
    slider_left->setStyleSheet(
        R"(
    QSlider::groove:vertical {
        border: 1px solid #555555;
        background: #999999;
        width: 8px;
        margin: 4px 0;
        border-radius: 4px;
    }
    QSlider::handle:vertical {
        background: #5c5c5c;
        border: 1px solid #1a1a1a;
        width: 16px;
        height: 30px;
        margin: -5px -4px;
        border-radius: 3px; /* Rectangular with slightly rounded corners */
    }
    QSlider::add-page:vertical, QSlider::sub-page:vertical {
        background: #999999;
        border-radius: 4px;
    }
    )"
        );

    slider_right->setStyleSheet(
        R"(
    QSlider::groove:vertical {
        border: 1px solid #555555;
        background: #999999;
        width: 8px;
        margin: 4px 0;
        border-radius: 4px;
    }
    QSlider::handle:vertical {
        background: #5c5c5c;
        border: 1px solid #1a1a1a;
        width: 16px;
        height: 30px;
        margin: -5px -4px;
        border-radius: 3px; /* Rectangular with slightly rounded corners */
    }
    QSlider::add-page:vertical, QSlider::sub-page:vertical {
        background: #999999;
        border-radius: 4px;
    }
    )"
        );

    // Set range (x10 because decimal precision)
    slider_left->setRange(0, 1000);
    slider_right->setRange(0, 1000);

    layout_actuator_slider->addWidget(slider_left);
    layout_actuator_slider->addWidget(slider_right);
    layout_actuator_frame->addLayout(layout_actuator_slider);

    // Link spinbox with slider

    QObject::connect(spinbox_left, QOverload<double>::of(&QDoubleSpinBox::valueChanged), slider_left, [=](double value){
        slider_left->setValue(static_cast<int>(value * 10));
    });

    QObject::connect(slider_left, &QSlider::valueChanged, spinbox_left, [=](int value){
        spinbox_left->setValue(value / 10.0);
    });

    QObject::connect(spinbox_right, QOverload<double>::of(&QDoubleSpinBox::valueChanged), slider_right, [=](double value){
        slider_right->setValue(static_cast<int>(value * 10));
    });

    QObject::connect(slider_right, &QSlider::valueChanged, spinbox_right, [=](int value){
        spinbox_right->setValue(value / 10.0);
    });

    //////////////////////////////////////////////////////////////////////////////
    /// Determine 3D equation for central angle computation
    /// Make the switch between angle and length (range of slider-spinbox)
    /// Define limits for warnings
    //////////////////////////////////////////////////////////////////////////////

    //radio_button1.toggled.connect(update_ranges)
    //radio_button2.toggled.connect(update_ranges)

    // This label should be use to display the central angle value and warn user if interfering angles
    // See python version l1429 at https://github.com/Innovative-Propulsion-Laboratory/Velaryon-IHM-Python/blob/main/main.py
    QLabel* angle_central_label = new QLabel("Control Panel", actuator_frame);

    angle_central_label->setFont(font);
    angle_central_label->setAlignment(Qt::AlignCenter);

    layout_actuator_frame->addWidget(angle_central_label);

    layout_actuator_frame->setSpacing(10);

    // This button should be linked to the UdpSender to send values types (length/angles) and content
    QPushButton* send_actuator_button = new QPushButton("Send Data",actuator_frame);
    send_actuator_button->setStyleSheet(R"(
        QPushButton {
            border: 1px solid black;
            background-color: white;
            border-radius: 4px;
            height: 30px;
        }
        QPushButton:hover {
            background-color: #ADADAD;
            border: 2px rgb(255,200,200);
        }
    )");

    layout_actuator_frame->addWidget(send_actuator_button);

    // Unavailable display (should be remove in future updates)
    QLabel* actuator_disabled = new QLabel(test_tab);
    actuator_disabled->setStyleSheet("background-color: rgba(128, 128, 128, 100);;");
    actuator_disabled->setGeometry(QRect(scale_x(411), scale_x(10), scale_x(200), scale_x(341)));

    // Create a pixmap to draw on
    QPixmap pixmap(actuator_disabled->size());
    pixmap.fill(Qt::transparent);  // Transparent background

    // Draw the rotated text
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.translate(pixmap.width() / 2, pixmap.height() / 2);
    painter.rotate(-45);  // Rotate counterclockwise

    font.setBold(true);
    font.setPointSize(20);  // Adjust size as needed
    painter.setFont(font);
    painter.setPen(Qt::black);  // Text color

    painter.drawText(QRect(-pixmap.width()/2, -pixmap.height()/2, pixmap.width(), pixmap.height()),
                     Qt::AlignCenter, "UNAVAILABLE");

    // Apply the pixmap to the label
    actuator_disabled->setPixmap(pixmap);
    // End of the unavailability display (do not remove further lines)

    //------------------------------------------------------------------------
    // TVC control frame (work must be done for proper display and code efficiency)
    QFrame* TVC_control_frame = new QFrame(test_tab);
    TVC_control_frame->setFrameShape(QFrame::StyledPanel);
    TVC_control_frame->setGeometry(QRect(scale_x(612), scale_x(10), scale_x(250), scale_x(240)));

    QVBoxLayout* frame_TVC_control = new QVBoxLayout(TVC_control_frame);
    frame_TVC_control->setAlignment(Qt::AlignCenter);

    // Title label for TVC control
    QLabel* TVC_label = new QLabel("TVC control", TVC_control_frame);
    TVC_label->setAlignment(Qt::AlignCenter);
    TVC_label->setStyleSheet("QLabel { font-size: 14px; font-weight: bold; margin-bottom: 10px; }");
    frame_TVC_control->addWidget(TVC_label);

    // Combobox to choose the desired pattern
    QHBoxLayout* layout_combobox_TVC = new QHBoxLayout();
    layout_combobox_TVC->setAlignment(Qt::AlignCenter);

    QComboBox* combobox_TVC = new QComboBox(TVC_control_frame);
    combobox_TVC->setFixedSize(scale_x(120), scale_x(30));
    combobox_TVC->addItems({"None", "Cross", "Circle", "Square", "Up-Down", "Left-Right"});
    combobox_TVC->setStyleSheet(R"(
    QComboBox {
        background-color: white;
        color: black;
        border: 2px solid #696969;
        border-radius: 5px;
        padding: 5px;
        font-size: 14px;
        padding-left: 10px;
    }
    QComboBox::drop-down {
        width: 25px;
        height: 26px;
        background-color: lightgray;
        border: none;
        border-radius: 2px;
    }
    QComboBox::down-arrow {
        width: 16px;
        height: 16px;
        image: url(:/ressources/pictures/arrow_down.png);
    }
    QComboBox QAbstractItemView {
        background-color: #f0f0f0;
        border: 1px solid #777777;
        selection-background-color: black;
        selection-color: black;
        outline: 0px;
        padding: 5px;
    }
    QComboBox QAbstractItemView::item:selected {
        background-color: #f0f0f0;
        padding: 5px;
        border-left: none;
    }
    QComboBox QAbstractItemView::item:focus {
        outline: none;
        padding: 5px;
    }
    QComboBox QAbstractItemView::item:hover {
        background-color: #e0e0e0;
        padding: 5px;
    })");
    layout_combobox_TVC->addWidget(combobox_TVC);
    layout_combobox_TVC->addStretch();

    // Layout originally use for two buttons - might be useless depending on future choices
    QVBoxLayout* buttonLayout = new QVBoxLayout();

    // Send a TVC command to test a pattern ( a second might be required for sending pattern to uC)
    QPushButton* patterntest = new QPushButton("Test pattern", TVC_control_frame);
    patterntest->setFixedSize(scale_x(100), scale_x(30));
    patterntest->setStyleSheet(R"(
        QPushButton {
            border: 1px solid black;
            background-color: white;
            border-radius: 4px;
            height: 30px;
        }
        QPushButton:hover {
            background-color: #ADADAD;
            border: 2px rgb(255,200,200);
        }
    )");
    buttonLayout->addWidget(patterntest);

    layout_combobox_TVC->addLayout(buttonLayout);

    frame_TVC_control->addLayout(layout_combobox_TVC);

    // Display a visual of the pattern
    QCustomPlot* customPlot = new QCustomPlot(TVC_control_frame);
    customPlot->setFixedSize(scale_x(100), scale_x(100));
    customPlot->xAxis->setVisible(false);
    customPlot->yAxis->setVisible(false);

    frame_TVC_control->addWidget(customPlot, 0, Qt::AlignCenter);

    customPlot->setBackground(QColor("#F3F3F3"));
    QObject::connect(combobox_TVC, &QComboBox::currentTextChanged, [=]() {
        customPlot->clearPlottables();
        QString selectedShape = combobox_TVC->currentText();

        UdpSender* sender = new UdpSender();

        // Clear previous items and graphs
        customPlot->clearGraphs();
        customPlot->clearItems();

        // Drawing
        if (selectedShape == "Cross") {
            QVector<double> x, y;

            // Define the two lines that make up the cross
            x << -1 << 1;
            y << 0 << 0; // Horizontal line

            customPlot->addGraph();
            customPlot->graph(0)->setData(x, y);
            customPlot->graph(0)->setPen(QPen(Qt::black, 2));

            QVector<double> x2, y2;
            x2 << 0 << 0;
            y2 << -1 << 1; // Vertical line

            // Add grap and provide points
            customPlot->addGraph();
            customPlot->graph(1)->setData(x2, y2);
            customPlot->graph(1)->setPen(QPen(Qt::black, 2));
            sender->sendTVCtest(1);     // Sends TVC pattern to uC
        }
        else if (selectedShape == "Circle") {
            // Draw a curve/spline instead of lines
            QCPCurve *circle = new QCPCurve(customPlot->xAxis, customPlot->yAxis);
            QVector<QCPCurveData> dataCircle;
            for (int i = 0; i < 101; ++i) {
                double theta = 2 * M_PI * i / 100;
                dataCircle.push_back(QCPCurveData(i, 1 * cos(theta), 1 * sin(theta)));
            }
            circle->data()->set(dataCircle, true);
            circle->setPen(QPen(Qt::black, 2));
            sender->sendTVCtest(2);
        }
        else if (selectedShape == "Square") {
            // Draw a rectangle using two points instead of 4
            QCPItemRect *rect = new QCPItemRect(customPlot);
            rect->topLeft->setCoords(-1, 1);
            rect->bottomRight->setCoords(1, -1);
            rect->setPen(QPen(Qt::black, 2));  // Outline color
            sender->sendTVCtest(3);
        }
        else if (selectedShape == "Up-Down") {
            QVector<double> x, y;
            x << 0 << 0;
            y << -1 << 1;
            customPlot->addGraph();
            customPlot->graph(0)->setData(x, y);
            customPlot->graph(0)->setPen(QPen(Qt::black, 2));
            sender->sendTVCtest(4);
        }
        else if (selectedShape == "Left-Right") {
            QVector<double> x, y;
            x << -1 << 1;
            y << 0 << 0;
            customPlot->addGraph();
            customPlot->graph(0)->setData(x, y);
            customPlot->graph(0)->setPen(QPen(Qt::black, 2));
            sender->sendTVCtest(5);
        }
        else {
            // Handle the case where no valid shape is selected
        }

        // Rescale axes to fit the shapes
        customPlot->xAxis->setRange(-1.05, 1.05);
        customPlot->yAxis->setRange(-1.05, 1.05);
        customPlot->replot();
    });

    // Similarlw to above, the next section creates an unavailability display (remove for future updates
    QLabel* tvc_disabled = new QLabel(test_tab);
    tvc_disabled->setStyleSheet("background-color: rgba(128, 128, 128, 100);;");
    tvc_disabled->setGeometry(QRect(scale_x(612), scale_x(10),
                                    scale_x(250), scale_x(240)));

    // Create a pixmap to draw on
    QPixmap pixmap_tvc(tvc_disabled->size());
    pixmap_tvc.fill(Qt::transparent);  // Transparent background

    // Draw the rotated text
    QPainter painter_tvc(&pixmap_tvc);
    painter_tvc.setRenderHint(QPainter::Antialiasing);
    painter_tvc.translate(pixmap_tvc.width() / 2, pixmap_tvc.height() / 2);
    painter_tvc.rotate(-45);  // Rotate counterclockwise

    font.setBold(true);
    font.setPointSize(20);  // Adjust size as needed
    painter_tvc.setFont(font);
    painter_tvc.setPen(Qt::black);  // Text color

    painter_tvc.drawText(QRect(-pixmap_tvc.width()/2, -pixmap_tvc.height()/2, pixmap_tvc.width(), pixmap_tvc.height()),
                     Qt::AlignCenter, "UNAVAILABLE");

    // Apply the pixmap to the label
    tvc_disabled->setPixmap(pixmap_tvc);
    // End of the unavailability section (do not remove further lines)

    //------------------------------------------------------------------------
    // Launch frame
    QFrame* launch_frame = new QFrame(test_tab);
    launch_frame->setGeometry(QRect(scale_x(10), scale_x(352),
                                    scale_x(851), scale_x(255)));
    launch_frame->setFrameShape(QFrame::StyledPanel);

    QVBoxLayout* launch_frame_layout = new QVBoxLayout;

    // Title + File Browser (explorateur)
    QHBoxLayout* title_layout = new QHBoxLayout;
    title_layout->setAlignment(Qt::AlignCenter);

    QLabel* title_label_launch = new QLabel("Launch test:");
    title_label_launch->setStyleSheet("font-weight: bold; font-size: 16px;");

    // Affiche le fichier sélectionné
    QLabel* selected_file_label = new QLabel("No file selected");
    selected_file_label->setFixedWidth(scale_x(350));
    selected_file_label->setStyleSheet("QLabel { background: white; border: 1px solid #696969; padding: 6px; border-radius: 5px; }");

    // Bouton pour ouvrir l'explorateur
    QPushButton* browse_button = new QPushButton("Browse...", launch_frame);
    browse_button->setFixedSize(scale_x(120), scale_x(30));
    browse_button->setStyleSheet(R"(
        QPushButton {
            border: 1px solid black;
            background-color: white;
            border-radius: 4px;
            height: 30px;
            font-size: 12px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #ADADAD;
            border: 2px solid rgb(0, 0, 0);
        }
    )");

    title_layout->addWidget(title_label_launch);
    title_layout->addSpacing(scale_x(10));
    title_layout->addWidget(selected_file_label);
    title_layout->addSpacing(scale_x(10));
    title_layout->addWidget(browse_button);

    // Grid layout to display sequence parameters (up to 30 variables - might need improvments if additional variables are required)
    QGridLayout* labels_grid = new QGridLayout;
    labels_grid->setSpacing(10);

    // Create a 5x6 grid but leave it empty initially
    for (int col = 0; col < 5; ++col) {
        for (int row = 0; row < 6; ++row) {
            QLabel* empty_label = new QLabel("---:---");  // Empty label initially
            empty_label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            empty_label->setContentsMargins(30, 0, 0, 0);
            labels_grid->addWidget(empty_label, row, col);
        }
    }

    // Stocker le chemin sélectionné
    auto selectedSequencePath = QSharedPointer<QString>::create(QString());

    // Ouvre l'explorateur et charge la séquence
    QObject::connect(browse_button, &QPushButton::clicked, [=]() {
        QString filePath = QFileDialog::getOpenFileName(
            test_tab,
            "Select a sequence file",
            QDir::homePath(),                       // dossier de départ
            "Sequence files (*.txt);;All files (*.*)"
            );

        if (filePath.isEmpty())
            return;

        *selectedSequencePath = filePath;
        selected_file_label->setText(QFileInfo(filePath).fileName());

        update_labels_from_file(labels_grid, filePath);
    });

    launch_frame_layout->addLayout(title_layout);
    launch_frame_layout->addLayout(labels_grid);
    launch_frame->setLayout(launch_frame_layout);

    // Button to send the sequence to the uC
    QPushButton* launch_button = new QPushButton("Start test");
    launch_button->setFixedSize(scale_x(150), scale_x(50));
    launch_button->setStyleSheet(R"(
        QPushButton {
            border: 1px solid black;
            background-color: white;
            border-radius: 3px;
            height: 30px;
            font-size: 12px;
            font-weight: bold;
            margin-bottom: 10px;
        }
        QPushButton:hover {
            background-color: #ADADAD;
            border: 2px solid rgb(0, 0, 0);
        }
    )");

    QObject::connect(launch_button, &QPushButton::clicked, []() {
        UdpSender sender;

        int count = 0;
        for (const auto& value : valuesList) {
            if (value == 999) {
                count++;
            }
        }

        // Abort if sequence file has dumb values (improvment: add a message box to inform user about dumb sequence file)
        if (count < 29)
        {
            QMessageBox::StandardButton reply;
            reply = QMessageBox::warning(nullptr, "Proceed with Sending?",
                                         "Are you sure you want to send the sequence data?",
                                         QMessageBox::Yes | QMessageBox::No);

            // If the user clicks "No", abort the operation
            if (reply == QMessageBox::Yes)
            {
                // Create a stringlist for display in the log terminal
                QStringList formatted;
                formatted<<"Sequence sent";
                for (const QString& item : sequence_registering) {
                    QStringList parts = item.split(",");
                    if (parts.size() == 3) {
                        if (parts[0].size()<13)
                            formatted << QString("\t\t%1:\t\t%2 %3").arg(parts[0], parts[1], parts[2]);
                        else
                            formatted << QString("\t\t%1:\t%2 %3").arg(parts[0], parts[1], parts[2]);
                    }
                }

                QString result = formatted.join("\n");
                logMessage(globalLogTerminal,result);
                sender.sendsequence(valuesList);        // Send sequence values to uC
            }
        }
    });
    QHBoxLayout* button_layout = new QHBoxLayout;;
    button_layout->addWidget(launch_button);

    launch_frame_layout->addLayout(button_layout);

    // Create an invisible overlay label in engine/cooling tabs
    QLabel* overlay_label_engine = new QLabel(engine_tab);
    overlay_label_engine->setStyleSheet("background-color: transparent;");
    overlay_label_engine->setGeometry(QRect(scale_x(50),scale_x(50),scale_x(1365),scale_x(600)));
    QLabel* overlay_label_cooling = new QLabel(cooling_tab);
    overlay_label_cooling->setStyleSheet("background-color: transparent;");
    overlay_label_cooling->setGeometry(QRect(scale_x(50), scale_x(50), scale_x(1365),scale_x(600)));
    overlay_label_engine->setVisible(false);
    overlay_label_cooling->setVisible(false);

    // ID du test actif en DB (-1 si aucun) — partagé entre les deux branches du timer
    auto activeTestId = QSharedPointer<int>::create(-1);

    // Timer to constantly check for launch flag (True means uC ready) and end flag (True means end of the test)
    QTimer* timer = new QTimer(test_tab);

    QObject::connect(timer, &QTimer::timeout, [overlay_label_engine, overlay_label_cooling,
                                               dbManager, selectedSequencePath, activeTestId]() {
        if (launch_test)
        {
            // FIX: Clear the flag IMMEDIATELY before showing any dialog,
            // to prevent the 10ms timer from re-entering this block while
            // QMessageBox runs its own event loop. Without this fix, multiple
            // dialogs would appear and 0xBB×4 would be sent several times to
            // the Teensy, causing a recursive Sequence() call and a stack
            // overflow on the microcontroller.
            launch_test = false;

            // Last warning before fire
            QMessageBox::StandardButton reply;
            reply = QMessageBox::warning(nullptr, "Warning: Test Launch",
                                         "This is your final warning!\n\n"
                                         "By clicking 'Yes', you are about to launch the test.\n"
                                         "Are you sure you want to proceed?",
                                         QMessageBox::Yes | QMessageBox::No);

            if (reply == QMessageBox::Yes)
            {
                UdpSender sender;
                QByteArray data;
                for (int j = 0; j < 4; j++) {
                    data.append(static_cast<char>(0xBB));
                }
                sender.sendMessage(data);

                // Démarre la session de test dans la DB
                if (dbManager)
                    *activeTestId = dbManager->startTest(*selectedSequencePath);

                // Overlay to disable all controls during test (improvment: add one for test tab)
                overlay_label_engine->setVisible(true);
                overlay_label_cooling->setVisible(true);
            }
        }
        if (end_test)
        {
            // Clôture la session de test dans la DB
            if (dbManager && *activeTestId >= 0) {
                dbManager->endTest(*activeTestId);
                *activeTestId = -1;
            }

            // Enable the manual control of the testbench
            overlay_label_engine->setVisible(false);
            overlay_label_cooling->setVisible(false);
            end_test = false;
            logMessage(globalLogTerminal, "Test ended");
        }
    });
    timer->start(10);   // Check every 10ms


    //------------------------------------------------------------------------
    // Countdown frame
    QFrame* countdown_frame = new QFrame(test_tab);
    countdown_frame->setGeometry(QRect(scale_x(612), scale_x(251), scale_x(250), scale_x(100)));
    countdown_frame->setFrameShape(QFrame::StyledPanel);

    // Create the label
    QLabel* time_label = new QLabel("--.---", countdown_frame);
    time_label->setStyleSheet(R"(font-size: 24px;font-weight: bold;)");

    QVBoxLayout* layout = new QVBoxLayout(countdown_frame);
    layout->addWidget(time_label);
    layout->setAlignment(Qt::AlignCenter);

    countdown_frame->setLayout(layout);

    // Countdown before start of ignition
    QObject::connect(timer, &QTimer::timeout,[time_label](){
        int seconds = (countdown % 60000) / 1000;
        int milliseconds = qAbs(countdown % 1000);
        time_label->setText(QString::asprintf("%02d.%03d",seconds, milliseconds));
    });
    //------------------------------------------------------------------------
    // Emergency frame
    QFrame* emergency_frame = new QFrame(test_tab);
    emergency_frame->setGeometry(QRect(scale_x(10), scale_x(608), scale_x(851), scale_x(160)));
    emergency_frame->setFrameShape(QFrame::StyledPanel);

    QVBoxLayout* emergency_frame_layout = new QVBoxLayout();

    // Title label for emergency section
    QLabel* title_label = new QLabel("Test abortion");
    title_label->setStyleSheet("font-weight: bold; font-size: 16px;");
    title_label->setAlignment(Qt::AlignCenter);
    emergency_frame_layout->addWidget(title_label);

    QPushButton* emergency_button = new QPushButton("Abort test");
    emergency_button->setFixedSize(scale_x(150), scale_x(100));
    emergency_button->setStyleSheet(
        "QPushButton {"
        "    border: 1px solid rgb(255, 0, 0);"
        "    background-color: rgb(220, 0, 0);"
        "    border-radius: 20px;"
        "    height: 30px;"
        "    font-size: 12px;"
        "    font-weight: bold;"
        "    margin-bottom: 10px;"
        "}"
        "QPushButton:hover {"
        "    background-color: rgb(250, 0, 0);"
        "    border: 2px solid rgb(255, 0, 0);"
        "}"
        );

    // Connect button to abort function (only button available during a test)
    QObject::connect(emergency_button, &QPushButton::clicked, [=]() {
        overlay_label_engine->setVisible(false);
        overlay_label_cooling->setVisible(false);
        UdpSender* sender = new UdpSender();
        QByteArray data;
        for (int j = 0; j < 4; j++) {
            data.append(static_cast<char>(0xCC));  // Append 0xFF four times
        }
        sender->sendMessage(data);
        logMessage(globalLogTerminal,"Test aborted");
    });

    emergency_frame_layout->addWidget(emergency_button, 0, Qt::AlignCenter);
    emergency_frame->setLayout(emergency_frame_layout);

    //------------------------------------------------------------------------
    // Initialise the log terminal
    QPlainTextEdit* logTerminal = new QPlainTextEdit(test_tab);
    globalLogTerminal = logTerminal;
    logTerminal->setGeometry(QRect(scale_x(862), scale_x(10), scale_x(670), scale_x(758)));
    logTerminal->setMaximumBlockCount(500);     // Set a limit to the number of lines
    logTerminal->setReadOnly(true);             // Makes the terminal read-only
    logTerminal->setStyleSheet("background-color: black; color: white;");

    return test_tab;
}
