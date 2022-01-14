# This Python file uses the following encoding: utf-8
import sys
import os
import glob
import time
import asyncio
import platform

from PyQt5 import QtCore, QtWidgets
import breeze_resources

from bleak import BleakClient, BleakScanner

from form import Ui_main


class main(QtWidgets.QMainWindow):

    def __init__(self):
        super(main, self).__init__()
        self.ui = Ui_main()
        self.ui.setupUi(self)
        #Get BLE devices
        self.loop = asyncio.get_event_loop()
        self.loop.run_until_complete(self.get_BLE_devices())
        #Connect all GUI components
        self.connect_all_gui_components()
        #Disable buttons
        self.ui.pushButton_start.setEnabled(False)
        self.ui.pushButton_stop.setEnabled(False)

    #Connect all GUI components
    def connect_all_gui_components(self):
        self.ui.comboBox_ports.currentIndexChanged.connect(self.set_BLEdevice_mac_addr)
        self.ui.pushButton_start.clicked.connect(self.start)
        self.ui.pushButton_stop.clicked.connect(self.stop)
        self.ui.pushButton_update.clicked.connect(self.get_BLE_devices)

    #Set the port that is selected from the dropdown menu
    def set_BLEdevice_mac_addr(self):
        self.BLEdevice_mac_addr = self.ui.comboBox_ports.currentText().split(":")[0]
        print(self.BLEdevice_mac_addr)
        #Enable buttons
        self.ui.pushButton_start.setEnabled(True)
        self.ui.pushButton_stop.setEnabled(True)

    #Update BLE decives
    def update_BLE_devices(self):
        self.ui.comboBox_ports.clear()
        self.loop = asyncio.get_event_loop()
        self.loop.run_until_complete(self.get_BLE_devices())

    #Scans all BLE decives
    async def get_BLE_devices(self):
        devices = []
        device = await BleakScanner.discover()
        for i in range(0,len(device)):
            print("["+str(i)+"]"+str(device[i]))
            devices.append(str(device[i]))
        self.ui.comboBox_ports.addItems(devices)

    #Start pump
    def start(self):
        #Fetch pump_1 parameter
        if self.ui.checkBox_1.isChecked():
            self.p1_velocity = self.ui.spinBox_velocity_p1.value()
            self.p1_volume = self.ui.spinBox_volume_p1.value()
            self.syringe_1 = self.ui.comboBox_syringe_1.currentIndex()
            self.steps_per_second_1 = self.calc_steps_per_second(self.p1_velocity, self.syringe_1)
            self.total_steps_1 = self.calc_total_steps(self.p1_volume, self.syringe_1)
            if self.steps_per_second_1 < 0:
                self.total_steps_1 = -self.total_steps_1
        else:
            self.steps_per_second_1 = 0
            self.total_steps_1 = 0
        #Fetch pump_2 parameter
        if self.ui.checkBox_2.isChecked():
            self.p2_velocity = self.ui.spinBox_velocity_p2.value()
            self.p2_volume = self.ui.spinBox_volume_p2.value()
            self.syringe_2 = self.ui.comboBox_syringe_2.currentIndex()
            self.steps_per_second_2 = self.calc_steps_per_second(self.p2_velocity, self.syringe_2)
            self.total_steps_2 = self.calc_total_steps(self.p2_volume, self.syringe_2)
            if self.steps_per_second_2 < 0:
                self.total_steps_2 = -self.total_steps_2
        else:
            self.steps_per_second_2 = 0
            self.total_steps_2 = 0
        #Send data to pump via BLE
        self.loop.run_until_complete(self.send_data())
        #Enable or disable Buttons
        self.ui.pushButton_start.setEnabled(False)
        self.ui.pushButton_stop.setEnabled(True)

    #Stop pump
    def stop(self):        
        #Send stop data to pump via BLE
        self.total_steps_1 = 0
        self.steps_per_second_1 = 0
        self.total_steps_2 = 0
        self.steps_per_second_2 = 0
        self.loop.run_until_complete(self.send_data())
        #Enable or disable Buttons
        self.ui.pushButton_start.setEnabled(True)

    def calc_steps_per_second(self, velocity, syringe_index):
        self.microsteps = self.ui.comboBox_microsteps.currentText()
        # syringe_parameters = steps needed per uL when microstepping=1
        # BD 1mL = 14.53, 3mL, 5mL, 10mL, 15mL
        syringe_parameters = [4.47, 2.7, 1.35, 0.9]
        volume_per_second = float(velocity)/60
        steps_per_second = round(volume_per_second*syringe_parameters[syringe_index]*int(self.microsteps))
        return steps_per_second

    def calc_total_steps(self,volume, syringe_index):
        self.microsteps = self.ui.comboBox_microsteps.currentText()
        # syringe_parameters = steps needed per uL when microstepping=1
        # BD 1mL = 14.53, 3mL, 5mL, 10mL, 15mL
        syringe_3mL = self.ui.doubleSpinBox_3mL.value()
        syringe_5mL = self.ui.doubleSpinBox_5mL.value()
        syringe_10mL = self.ui.doubleSpinBox_10mL.value()
        syringe_15mL = self.ui.doubleSpinBox_15mL.value()
        syringe_parameters = [syringe_3mL, syringe_5mL, syringe_10mL, syringe_15mL]
        total_steps = volume*syringe_parameters[syringe_index]*int(self.microsteps)
        return total_steps

    async def send_data(self):
        #Send data to one specific BLE device
        #Data is send in seperate bytes, while 10 bytes from a "full package" (variable value)
        device = await BleakScanner.find_device_by_address(self.BLEdevice_mac_addr)
        async with BleakClient(device) as client:
            #Step data pump 1
            data_steps_1 = str(self.total_steps_1)
            for x in range(10-len(data_steps_1)):
                await client.write_gatt_char("45845fe7-633a-463f-b4ab-5f0ed8403544", b'0')
                await asyncio.sleep(0.05)
            for x in range(len(data_steps_1)):
                substring = data_steps_1[x:x+1]
                substring_as_bytes = str.encode(substring)
                await client.write_gatt_char("45845fe7-633a-463f-b4ab-5f0ed8403544", substring_as_bytes)
                await asyncio.sleep(0.05)
            #Speed data pump 1
            data_speed_1 = str(self.steps_per_second_1)
            for x in range(10-len(data_speed_1)):
                await client.write_gatt_char("039ee0a6-6db7-46de-aad3-0eaa483918a1", b'0')
                await asyncio.sleep(0.05)
            for x in range(len(data_speed_1)):
                substring = data_speed_1[x:x+1]
                substring_as_bytes = str.encode(substring)
                await client.write_gatt_char("039ee0a6-6db7-46de-aad3-0eaa483918a1", substring_as_bytes)
                await asyncio.sleep(0.05)
            #Step data pump 2
            data_steps_2 = str(self.total_steps_2)
            for x in range(10-len(data_steps_2)):
                await client.write_gatt_char("7853b997-bbda-4d6f-842d-d00f97643699", b'0')
                await asyncio.sleep(0.05)
            for x in range(len(data_steps_2)):
                substring = data_steps_2[x:x+1]
                substring_as_bytes = str.encode(substring)
                await client.write_gatt_char("7853b997-bbda-4d6f-842d-d00f97643699", substring_as_bytes)
                await asyncio.sleep(0.05)
            #Speed data pump 2
            data_speed_2 = str(self.steps_per_second_2)
            for x in range(10-len(data_speed_2)):
                await client.write_gatt_char("bcae9061-246b-451f-88bb-68ac8af74317", b'0')
                await asyncio.sleep(0.05)
            for x in range(len(data_speed_2)):
                substring = data_speed_2[x:x+1]
                substring_as_bytes = str.encode(substring)
                await client.write_gatt_char("bcae9061-246b-451f-88bb-68ac8af74317", substring_as_bytes)
                await asyncio.sleep(0.05)
            print("Total steps for pump 1: " + data_steps_1)
            print("Speed for pump 1: " + data_speed_1)
            print("Total steps for pump 2: " + data_steps_2)
            print("Speed for pump 2: " + data_speed_2)

if __name__ == "__main__":
    app = QtWidgets.QApplication(sys.argv)

    #Set Stylesheet
    file = QtCore.QFile(":/dark.qss")
    file.open(QtCore.QFile.ReadOnly | QtCore.QFile.Text)
    stream = QtCore.QTextStream(file)
    app.setStyleSheet(stream.readAll())

    window = main()
    window.show()
    sys.exit(app.exec_())
