import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    visible: true
    width: 1024
    height: 768
    title: "SCADA Dashboard"
    id: mainWindow

    property var bridge: null

    property real temperature: 25.0
    property real pressure: 1013.25
    property real humidity: 45.0
    property bool pump1Active: false
    property bool pump2Active: false
    property bool alarmActive: false
    property int tankLevel: 75

    function formatTemperature(temp) {
        if (isNaN(temp)) return "--°C"
        if (Math.abs(temp) >= 100) {
            return Math.round(temp) + "°C"
        } else {
            return temp.toFixed(1) + "°C"
        }
    }

    function formatPressure(press) {
        if (isNaN(press)) return "-- hPa"
        if (press >= 1000) {
            return press.toFixed(1) + " hPa"
        } else {
            return press.toFixed(0) + " hPa"
        }
    }

    function formatHumidity(hum) {
        if (isNaN(hum)) return "--%"
        return Math.round(Math.max(0, Math.min(100, hum))) + "%"
    }

    function formatTankLevel(level) {
        if (isNaN(level)) return "--%"
        return Math.max(0, Math.min(100, level)) + "%"
    }

    Rectangle {
        anchors.fill: parent
        color: Qt.color("#2e2e2e")

        GridLayout {
            anchors.fill: parent
            anchors.margins: 20
            columns: 3
            rowSpacing: 20
            columnSpacing: 20

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: Qt.color("#3e3e3e")
                radius: 10

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10

                    Label {
                        text: "Temperature"
                        font.pixelSize: 20
                        color: Qt.color("#ffffff")
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: Qt.color("transparent")
                        
                        Text {
                            anchors.centerIn: parent
                            text: mainWindow.formatTemperature(mainWindow.temperature)
                            font.pixelSize: Math.min(40, parent.width / 6)
                            color: mainWindow.temperature > 50 ? Qt.color("#ff0000") : Qt.color("#ffffff")
                        }
                    }

                    ProgressBar {
                        Layout.fillWidth: true
                        from: -20
                        to: 100
                        value: mainWindow.temperature
                        id: tempProgressBar

                        background: Rectangle {
                            implicitWidth: 200
                            implicitHeight: 20
                            color: Qt.color("#1e1e1e")
                            radius: 3
                        }

                        contentItem: Item {
                            implicitWidth: 200
                            implicitHeight: 20

                            Rectangle {
                                width: parent.width * Math.max(0, Math.min(1, (mainWindow.temperature - tempProgressBar.from) / (tempProgressBar.to - tempProgressBar.from)))
                                height: parent.height
                                radius: 3
                                color: mainWindow.temperature > 50 ? Qt.color("#ff0000") : Qt.color("#00aaff")
                            }
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: Qt.color("#3e3e3e")
                radius: 10

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10

                    Label {
                        text: "Pressure"
                        font.pixelSize: 20
                        color: Qt.color("#ffffff")
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: Qt.color("transparent")
                        
                        Text {
                            anchors.centerIn: parent
                            text: mainWindow.formatPressure(mainWindow.pressure)
                            font.pixelSize: Math.min(40, parent.width / 7)
                            color: Qt.color("#ffffff")
                        }
                    }

                    ProgressBar {
                        Layout.fillWidth: true
                        from: 900
                        to: 1100
                        value: mainWindow.pressure
                        id: pressureProgressBar

                        background: Rectangle {
                            implicitWidth: 200
                            implicitHeight: 20
                            color: Qt.color("#1e1e1e")
                            radius: 3
                        }

                        contentItem: Item {
                            implicitWidth: 200
                            implicitHeight: 20

                            Rectangle {
                                width: parent.width * Math.max(0, Math.min(1, (mainWindow.pressure - pressureProgressBar.from) / (pressureProgressBar.to - pressureProgressBar.from)))
                                height: parent.height
                                radius: 3
                                color: Qt.color("#00ff00")
                            }
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: Qt.color("#3e3e3e")
                radius: 10

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10

                    Label {
                        text: "Humidity"
                        font.pixelSize: 20
                        color: Qt.color("#ffffff")
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: Qt.color("transparent")
                        
                        Text {
                            anchors.centerIn: parent
                            text: mainWindow.formatHumidity(mainWindow.humidity)
                            font.pixelSize: Math.min(40, parent.width / 5)
                            color: Qt.color("#ffffff")
                        }
                    }

                    ProgressBar {
                        Layout.fillWidth: true
                        from: 0
                        to: 100
                        value: mainWindow.humidity
                        id: humidityProgressBar

                        background: Rectangle {
                            implicitWidth: 200
                            implicitHeight: 20
                            color: Qt.color("#1e1e1e")
                            radius: 3
                        }

                        contentItem: Item {
                            implicitWidth: 200
                            implicitHeight: 20

                            Rectangle {
                                width: parent.width * Math.max(0, Math.min(1, (mainWindow.humidity - humidityProgressBar.from) / (humidityProgressBar.to - humidityProgressBar.from)))
                                height: parent.height
                                radius: 3
                                color: Qt.color("#4488ff")
                            }
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.columnSpan: 2
                color: Qt.color("#3e3e3e")
                radius: 10

                GridLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    columns: 2

                    Label {
                        text: "Pump Status"
                        font.pixelSize: 20
                        color: Qt.color("#ffffff")
                        Layout.columnSpan: 2
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: Qt.color("transparent")
                        Layout.alignment: Qt.AlignHCenter

                        Column {
                            spacing: 10
                            anchors.centerIn: parent

                            Rectangle {
                                width: 24
                                height: 24
                                radius: 12
                                color: mainWindow.pump1Active ? Qt.color("#00ff00") : Qt.color("#ff4444")
                                border.color: Qt.color("#ffffff")
                                border.width: 2
                                
                                SequentialAnimation on scale {
                                    running: mainWindow.pump1Active
                                    loops: Animation.Infinite
                                    NumberAnimation { to: 1.2; duration: 800; easing.type: Easing.InOutQuad }
                                    NumberAnimation { to: 1.0; duration: 800; easing.type: Easing.InOutQuad }
                                }
                            }

                            Text {
                                text: "Pump 1: " + (mainWindow.pump1Active ? "ON" : "OFF")
                                color: Qt.color("#ffffff")
                                font.pixelSize: Math.min(14, parent.parent.parent.width / 15)
                                font.weight: Font.Bold
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: Qt.color("transparent")
                        Layout.alignment: Qt.AlignHCenter

                        Column {
                            spacing: 10
                            anchors.centerIn: parent

                            Rectangle {
                                width: 24
                                height: 24
                                radius: 12
                                color: mainWindow.pump2Active ? Qt.color("#00ff00") : Qt.color("#ff4444")
                                border.color: Qt.color("#ffffff")
                                border.width: 2
                                
                                SequentialAnimation on scale {
                                    running: mainWindow.pump2Active
                                    loops: Animation.Infinite
                                    NumberAnimation { to: 1.2; duration: 800; easing.type: Easing.InOutQuad }
                                    NumberAnimation { to: 1.0; duration: 800; easing.type: Easing.InOutQuad }
                                }
                            }

                            Text {
                                text: "Pump 2: " + (mainWindow.pump2Active ? "ON" : "OFF")
                                color: Qt.color("#ffffff")
                                font.pixelSize: Math.min(14, parent.parent.parent.width / 15)
                                font.weight: Font.Bold
                            }
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: Qt.color("#3e3e3e")
                radius: 10

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10

                    Label {
                        text: "Alarm"
                        font.pixelSize: 20
                        color: Qt.color("#ffffff")
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: mainWindow.alarmActive ? Qt.color("#ff0000") : Qt.color("#3e3e3e")
                        radius: 10
                        border.color: mainWindow.alarmActive ? Qt.color("#ff0000") : Qt.color("#666666")
                        border.width: 2

                        SequentialAnimation on opacity {
                            running: mainWindow.alarmActive
                            loops: Animation.Infinite
                            NumberAnimation { to: 0.3; duration: 1000 }
                            NumberAnimation { to: 1.0; duration: 1000 }
                        }

                        Text {
                            anchors.centerIn: parent
                            text: mainWindow.alarmActive ? "¡ALARM!" : "Normal"
                            color: Qt.color("#ffffff")
                            font.pixelSize: Math.min(24, parent.width / 8)
                            font.weight: Font.Bold
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.columnSpan: 3
                color: Qt.color("#3e3e3e")
                radius: 10

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10

                    Label {
                        text: "Tank Level"
                        font.pixelSize: 20
                        color: Qt.color("#ffffff")
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 50
                        color: Qt.color("#1e1e1e")
                        radius: 8
                        border.color: Qt.color("#444444")
                        border.width: 1

                        Rectangle {
                            width: parent.width * Math.max(0, Math.min(1, mainWindow.tankLevel / 100))
                            height: parent.height
                            color: {
                                if (mainWindow.tankLevel < 20) return Qt.color("#ff4444")
                                if (mainWindow.tankLevel < 40) return Qt.color("#ff9900")
                                if (mainWindow.tankLevel < 80) return Qt.color("#44aa44")
                                return Qt.color("#00ff00")
                            }
                            radius: 8
                            
                            Behavior on width {
                                NumberAnimation { duration: 500; easing.type: Easing.OutQuart }
                            }
                            
                            Behavior on color {
                                ColorAnimation { duration: 300 }
                            }
                        }

                        Text {
                            anchors.centerIn: parent
                            text: mainWindow.formatTankLevel(mainWindow.tankLevel)
                            color: Qt.color("#ffffff")
                            font.pixelSize: Math.min(18, parent.width / 12)
                            font.weight: Font.Bold
                            style: Text.Outline
                            styleColor: Qt.color("#000000")
                        }
                    }
                }
            }
        }
    }
}
