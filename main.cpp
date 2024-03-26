// MCP2221 Demonstration Utility

#include "mainwindow.h"
#include <QApplication>
#include <QTimer>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QSignalMapper>
#include <QLineSeries>
#include <QPointF>
#include <QVector>
#include <QtCharts/QChartView>
#include <QtCharts/QValueAxis>
#include <QtCharts/QXYSeries>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QTextEdit>
#include <QThread>

#include "mcp2221_dll_um.h"
#include "Mcp22xx_GetErrorName.h"

//MCP2221 variables
#define I2cAddr7bit 1
#define I2cAddr8bit 0
void *handle;
wchar_t LibVer[6];
wchar_t MfrDescriptor[30];
wchar_t ProdDescrip[30];
int ver = 0;
int error = 0;
int flag = 0;
unsigned char pinFunc[4] = {MCP2221_GPFUNC_IO, MCP2221_GPFUNC_IO, MCP2221_GPFUNC_IO, MCP2221_GPFUNC_IO};  //Set GP0, GP1, GP3 as digital IO and GP2 as DAC
unsigned char pinDir[4] = {MCP2221_GPDIR_INPUT, MCP2221_GPDIR_INPUT, MCP2221_GPDIR_INPUT, MCP2221_GPDIR_INPUT};  //configure GP0, GP1, GP3 as digital output
unsigned char OutValues[4] = {MCP2221_GPVAL_LOW, MCP2221_GPVAL_LOW, MCP2221_GPVAL_LOW, MCP2221_GPVAL_LOW};   //set initial values to 0's
unsigned char PowerAttrib;
unsigned char DacVal = 31;
unsigned char DacRefValue = 0;
unsigned int ReqCurrent;
unsigned int PID = 0xDD;
unsigned int VID = 0x4D8;
unsigned int NumOfDev = 0;

uint8_t counter = 0;
uint8_t DacVref = 0, AdcVref = 0;
unsigned int analogReadings[3];
char TxBuffer[20];

unsigned char SineLUT64[] = {16,17,19,20,21,23,24,25,
                            26,27,28,29,30,30,31,31,
                            31,31,31,30,30,29,28,27,
                            26,25,24,23,21,20,19,17,
                            16,14,12,11,10,8,7,6,
                            5,4,3,2,1,1,0,0,
                            0,0,0,1,1,2,3,4,
                            5,6,7,8,10,11,12,14};

unsigned char SineLUT32[] = {16,19,21,24,26,28,30,31,
                            31,31,30,28,26,24,21,19,
                            16,12,10,7,5,3,1,0,
                            0,0,1,3,5,7,10,12};
// Qt variables
QTimer *timer;
QString string;
QComboBox *DacVrefDropList, *AdcVrefDropList, *GpioFunctionDropList, *PinDirDropList,*pinDropList;
//GPIO control checkboxes
QCheckBox *GPIO0chkBx;
QCheckBox *GPIO1chkBx;
QCheckBox *GPIO2chkBx;
QCheckBox *GPIO3chkBx;
QSerialPort* serial;
QComboBox *SerialPortDropList, *BaudRateDropList;
QTextEdit *TextBox, *ReadSerialTextBox, *I2CAddrTextBox;
QByteArray buffer;
unsigned char Addr = 0x00;
unsigned char DummyByte = 0x00;

//QChart variables
//using namespace QtCharts;
QChartView *chartViewAcc;
QLineSeries *seriesX;
QLineSeries *seriesY;
QLineSeries *seriesZ;
QChart *chartAcc;
QPointF numX, numY, numZ;
QTimer *LoggingTimer;

int count = 0;
double xVal = 0, yVal = 0, zVal = 0;
int x1RangeMax = 64, x1RangeMin = 0, y1RangeMax = 1023, y1RangeMin = 0;

void MainWindow::ExitApp()
{
    qDebug("Closing");
    QThread::msleep(10);
    //_sleep(10);
    //Mcp2221_Reset(handle);

    //Close all devices at exit
    //Mcp2221_CloseAll();

    Mcp2221_Close(handle);
}

void Mcp2221_config()
{
    ver = Mcp2221_GetLibraryVersion(LibVer);		//Get DLL version
    if(ver == 0)
    {
        qDebug("Library (DLL) version: %ls", LibVer);
        string = "Library (DLL) version: "+QString::fromWCharArray(LibVer);
    }
    else
    {
        error = Mcp2221_GetLastError();
        qDebug("Version can't be found, version: %d, error: %d", ver, error);
        string = "Cannot get version, error: " + QString::number(error) + " " + QString(Mcp2221_GetErrorName(error));
    }

    //Get number of connected devices with this VID & PID
    Mcp2221_GetConnectedDevices(VID, PID, &NumOfDev);
    if(NumOfDev == 0)
    {
        qDebug("No MCP2221 devices connected");
        string = "No MCP2221 devices connected";
    }
    else
    {
        qDebug("Number of devices found: %d", NumOfDev);
        string = "Number of devices found: " + QString::number(NumOfDev);
    }

    //Open device by index
    handle = Mcp2221_OpenByIndex(VID, PID, NumOfDev-1);
    error = Mcp2221_GetLastError();
    if(error == E_NO_ERR)
    {
        qDebug("Connection successful");
        string = "Connection successful";
    }
    else
    {
        //error = Mcp2221_GetLastError();
        qDebug("Error message is %d: %s", error, Mcp2221_GetErrorName(error));
        string = "Error message is "+ QString::number(error) + " " + QString(Mcp2221_GetErrorName(error));
    }

    //Get manufacturer descriptor
    flag = Mcp2221_GetManufacturerDescriptor(handle, MfrDescriptor);
    if(flag == 0)
    {
        qDebug("Manufacturer descriptor: %ls", MfrDescriptor);
        string = "Manufacturer descriptor: " + QString::fromWCharArray(MfrDescriptor);
    }
    else
    {
        qDebug("Error getting descriptor: %d", flag);
        string = "Error getting descriptor: " + QString::number(flag);
    }

    //Get product descriptor
    flag = Mcp2221_GetProductDescriptor(handle, ProdDescrip);
    if(flag == 0)
    {
        qDebug("Product descriptor: %ls", ProdDescrip);
        string = "Product descriptor: " + QString::fromWCharArray(ProdDescrip);
    }
    else
    {
        qDebug("Error getting product descriptor: %d", flag);
        string = "Error getting product descriptor:" + QString::number(flag);
    }

    //Get power attributes
    flag = Mcp2221_GetUsbPowerAttributes(handle, &PowerAttrib, &ReqCurrent);
    if(flag == 0)
    {
        qDebug("Power Attributes, %x\nRequested current units = %d\nRequested current(mA) = %d", PowerAttrib, ReqCurrent, ReqCurrent*2);
        string = "Power Attributes " + QString::number(PowerAttrib) + "\nRequested current units = " + QString::number(ReqCurrent) + "\nRequested current(mA) = " + QString::number(ReqCurrent*2);
    }
    else
    {
        qDebug("Error getting power attributes: %d", flag);
        string = "Error getting power attributes:"+ QString::number(flag);
    }

    //Set I2C bus
    flag = Mcp2221_SetSpeed(handle, 500000);    //set I2C speed to 400 KHz
    if(flag == 0)
    {
        qDebug("I2C is configured");
        string = "I2C is configured";
    }
    else
    {
        qDebug("Error setting I2C bus: %d", flag);
        string = "Error setting I2C bus:"+ QString::number(flag);
    }

    //Set I2C advanced parameters
    flag = Mcp2221_SetAdvancedCommParams(handle, 10, 100);  //10ms timeout, try 1000 times
    if(flag == 0)
    {
        qDebug("I2C advanced settings set");
        string = "I2C advanced settings set";
    }
    else
    {
        qDebug("Error setting I2C advanced settings: %d", flag);
        string = "Error setting I2C advanced settings:"+ QString::number(flag);
    }

    //Set GPIO
    flag = Mcp2221_SetGpioSettings(handle, RUNTIME_SETTINGS, pinFunc, pinDir, OutValues);
    if(flag != 0)
    {
        qDebug("Error setting GPIO, error: %d", flag);
        string = "Error setting GPIO, error: "+ QString::number(flag);
    }


    // Default Vref = VDD for DAC
    flag = Mcp2221_SetDacVref(handle, RUNTIME_SETTINGS, VREF_VDD);
    if(flag != 0)
        qDebug("Error setting DAC Vref for GP2, error code: %d (%s)", flag, Mcp2221_GetErrorName(flag));

//    // Default Vref = VDD for ADC
//    flag = Mcp2221_SetAdcVref(handle, RUNTIME_SETTINGS, VREF_VDD);
//    if(flag != 0)
//        qDebug("Error setting ADC Vref for GP2, error code: %d (%s)", flag, Mcp2221_GetErrorName(flag));

}

void MainWindow::TimerCallback()
{
	// Generate a sinusoidal waveform
    counter++;
    if(counter == 32)
        counter = 0;
    flag = Mcp2221_SetDacValue(handle, RUNTIME_SETTINGS, SineLUT32[counter]);  //must use "RUNTIME_SETTINGS" to enable and output voltage to the DAC,
    if(flag != 0)
    {
        qDebug("Error setting DAC, error: %d: %s", flag, Mcp2221_GetErrorName(flag));
    }

	// Plot data from ADC
    Mcp2221_GetAdcData(handle, analogReadings);
    count++;
    if(count == x1RangeMax)
    {
        count = 0;
        seriesX->clear();
        seriesY->clear();
        seriesZ->clear();
    }
    numX.setX(count);
    numX.setY(analogReadings[0]);
    seriesX->append(numX);
}

void MainWindow::ChangeVrefCallback(int index)
{
    timer->stop();	// stop the timer
    //Set DAC reference
    flag = Mcp2221_SetDacVref(handle, RUNTIME_SETTINGS, index);
    if(flag != 0)
    {
        qDebug("Error setting DAC reference, error: %d\n", flag);
    }
    timer->start();		//start the timer
}

// Apply settings to MCP2221 memory callback function
void MainWindow::ApplySettingsCallback()
{
    timer->stop();
    if(pinDropList->currentIndex() == 0)  //Pin GP0 selected
    {
        // GPIO function
        if(GpioFunctionDropList->currentIndex() == 0)
        {
            if(PinDirDropList->currentIndex() == 0)     //GP0 is set as input
            {
                pinFunc[0] = MCP2221_GPFUNC_IO;
                pinDir[0] = MCP2221_GPDIR_INPUT;
                qDebug("GP0: input");
            }
            else if(PinDirDropList->currentIndex() == 1)    //GP0 is set as output
            {
                pinFunc[0] = MCP2221_GPFUNC_IO;
                pinDir[0] = MCP2221_GPDIR_OUTPUT;
                qDebug("GP0: output");
            }
        }
    }
    if(pinDropList->currentIndex() == 1)  //Pin GP1 selected, alt. function: ADC1
    {
        // GPI1 function
        if(GpioFunctionDropList->currentIndex() == 0)
        {
            if(PinDirDropList->currentIndex() == 0)     //GP1 is set as input
            {
                pinFunc[1] = MCP2221_GPFUNC_IO;
                pinDir[1] = MCP2221_GPDIR_INPUT;
                qDebug("GP1: input");
            }
            else if(PinDirDropList->currentIndex() == 1)    //GP1 is set as output
            {
                pinFunc[1] = MCP2221_GPFUNC_IO;
                pinDir[1] = MCP2221_GPDIR_OUTPUT;
                qDebug("GP1: output");
            }
        }
        // ADC1 function for GP1
        if(GpioFunctionDropList->currentIndex() == 1)
        {
            pinFunc[1] = MCP2221_GP_ADC;
            pinDir[1] = NO_CHANGE;
            AdcVref = AdcVrefDropList->currentIndex();
            //flag = Mcp2221_SetAdcVref(handle, RUNTIME_SETTINGS, AdcVrefDropList->currentIndex());
            //if(flag != 0)
            //    qDebug("Error setting ADC Vref for GP1, error code: %d (%s)", flag, Mcp2221_GetErrorName(flag));
            qDebug("GP1: ADC");
        }
    }
    if(pinDropList->currentIndex() == 2)  //Pin GP2 selected, alternate function: ADC2, DAC1
    {
        // GPI2 function
        if(GpioFunctionDropList->currentIndex() == 0)
        {
            if(PinDirDropList->currentIndex() == 0)     //GP2 is set as input
            {
                pinFunc[2] = MCP2221_GPFUNC_IO;
                pinDir[2] = MCP2221_GPDIR_INPUT;
                qDebug("GP2: input");
            }
            else if(PinDirDropList->currentIndex() == 1)    //GP2 is set as output
            {
                pinFunc[2] = MCP2221_GPFUNC_IO;
                pinDir[2] = MCP2221_GPDIR_OUTPUT;
                qDebug("GP2: output");
            }
        }
        // ADC2 function for GP2
        if(GpioFunctionDropList->currentIndex() == 1)
        {
            pinFunc[2] = MCP2221_GP_ADC;
            pinDir[2] = NO_CHANGE;
            AdcVref = AdcVrefDropList->currentIndex();
            //flag = Mcp2221_SetAdcVref(handle, RUNTIME_SETTINGS, AdcVrefDropList->currentIndex());
            //if(flag != 0)
            //    qDebug("Error setting ADC Vref for GP2, error code: %d (%s)", flag, Mcp2221_GetErrorName(flag));
            qDebug("GP2: ADC");
        }

        // DAC1 function for GP2
        if(GpioFunctionDropList->currentIndex() == 2)
        {
            pinFunc[2] = MCP2221_GP_DAC;
            pinDir[2] = NO_CHANGE;
            DacVref = DacVrefDropList->currentIndex();
            //flag = Mcp2221_SetDacVref(handle, RUNTIME_SETTINGS, DacVrefDropList->currentIndex());
            //if(flag != 0)
            //    qDebug("Error setting DAC Vref for GP2, error code: %d (%s)", flag, Mcp2221_GetErrorName(flag));
            qDebug("GP2: DAC");
        }
    }
    if(pinDropList->currentIndex() == 3)  //Pin GP3 selected, alternate	function: ADC3, DAC2
    {
        // GPI3 function
        if(GpioFunctionDropList->currentIndex() == 0)
        {
            if(PinDirDropList->currentIndex() == 0)     //GP0 is set as input
            {
                pinFunc[3] = MCP2221_GPFUNC_IO;
                pinDir[3] = MCP2221_GPDIR_INPUT;
                qDebug("GP3: input");
            }
            else if(PinDirDropList->currentIndex() == 1)    //GP0 is set as output
            {
                pinFunc[3] = MCP2221_GPFUNC_IO;
                pinDir[3] = MCP2221_GPDIR_OUTPUT;
                qDebug("GP3: output");
            }
        }
        // ADC3 function for GP3
        if(GpioFunctionDropList->currentIndex() == 1)
        {
            pinFunc[3] = MCP2221_GP_ADC;
            pinDir[3] = NO_CHANGE;
            AdcVref = AdcVrefDropList->currentIndex();
            //flag = Mcp2221_SetAdcVref(handle, RUNTIME_SETTINGS, AdcVrefDropList->currentIndex());
            //if(flag != 0)
            //    qDebug("Error setting ADC Vref for GP3, error code: %d (%s)", flag, Mcp2221_GetErrorName(flag));
            qDebug("GP3: ADC");
        }

        // DAC2 function for GP3
        if(GpioFunctionDropList->currentIndex() == 2)
        {
            //timer->stop();
            pinFunc[3] = MCP2221_GP_DAC;
            pinDir[3] = MCP2221_GPDIR_OUTPUT;
            DacVref = DacVrefDropList->currentIndex();
            //flag = Mcp2221_SetDacVref(handle, RUNTIME_SETTINGS, DacVrefDropList->currentIndex());
            //if(flag != 0)
            //    qDebug("Error setting DAC Vref for GP2, error code: %d (%s)", flag, Mcp2221_GetErrorName(flag));
            qDebug("GP3: DAC");
        }
    }

    // set GPIO
    flag = Mcp2221_SetGpioSettings(handle, RUNTIME_SETTINGS, pinFunc, pinDir, OutValues);
    if(flag != 0)
    {
        qDebug("Error setting GPIO, error: %d", flag);
        string = "Error setting GPIO, error: "+ QString::number(flag);
    }
    QThread::msleep(10);

    flag = Mcp2221_SetGpioDirection(handle, pinDir);
    if(flag != 0)
        qDebug("Error setting GPIO direction, error code: %d (%s)", flag, Mcp2221_GetErrorName(flag));

    flag = Mcp2221_SetGpioValues(handle, OutValues);
    if(flag != 0)
        qDebug("Error setting GPIO output values, error code: %d (%s)", flag, Mcp2221_GetErrorName(flag));

    // set DAC Vref
    flag = Mcp2221_SetDacVref(handle, RUNTIME_SETTINGS, DacVref);
    if(flag != 0)
        qDebug("Error setting DAC Vref for GP2, error code: %d (%s)", flag, Mcp2221_GetErrorName(flag));
    qDebug("DAC Vref: %d", DacVref);
    QThread::msleep(10);

    // set ADC Vref
    flag = Mcp2221_SetAdcVref(handle, RUNTIME_SETTINGS, AdcVref);
    if(flag != 0)
        qDebug("Error setting DAC Vref for GP2, error code: %d (%s)", flag, Mcp2221_GetErrorName(flag));
    qDebug("ADC Vref: %d", AdcVref);

    QThread::msleep(50);
    timer->start();
}

// Digital output callback function
void MainWindow::DigitalOutCallback(int index)
{
    if(GPIO0chkBx->isChecked())
        OutValues[0] = 1;
    else
        OutValues[0] = 0;

    if(GPIO1chkBx->isChecked())
        OutValues[1] = 1;
    else
        OutValues[1] = 0;

    if(GPIO2chkBx->isChecked())
        OutValues[2] = 1;
    else
        OutValues[2] = 0;

    if(GPIO3chkBx->isChecked())
        OutValues[3] = 1;
    else
        OutValues[3] = 0;
    Mcp2221_SetGpioValues(handle, OutValues);
}

void MainWindow::ConnectSerialCallback()
{
    // Configure serial port 
    serial->setPortName(SerialPortDropList->currentText());
    serial->setBaudRate(BaudRateDropList->currentText().toInt());
    serial->setDataBits(QSerialPort::Data8);
    serial->setParity(QSerialPort::NoParity);
    serial->setStopBits(QSerialPort::OneStop);
    serial->setFlowControl(QSerialPort::NoFlowControl);
    serial->setReadBufferSize(65536);
    serial->open(QIODevice::ReadWrite);
    QObject::connect(serial, SIGNAL(readyRead()), this, SLOT(ReadSerialPort()));
    qDebug() << "Port" << SerialPortDropList->currentText() << "selected at baurate of" << BaudRateDropList->currentText() << "bps";
}

// Callback function for sending data to serial (UART)
void MainWindow::SendSerial()
{
    sprintf(TxBuffer, "%s", TextBox->toPlainText().toStdString().c_str());
    serial->write(TxBuffer);
}

// Callback function for receiving data from serial port (UART)
void MainWindow::ReadSerialPort()
{
    buffer = serial->readAll();
    ReadSerialTextBox->append(buffer);
    buffer.clear();
}

// I2C scanner callback function
void MainWindow::Mcp2221_ScanI2cBus()
{
    //Scan all I2C addresses from 1 to 127
    for (Addr = 1; Addr <= 127; Addr++)
    {
        //issue start condition then address
        flag = Mcp2221_I2cWrite(handle, sizeof(DummyByte), Addr, I2cAddr7bit, &DummyByte);
        if (flag == 0)
        {
            string = "Device found: 0x" + QString("%1").arg((uint8_t)Addr, 0, 16); //QString::number(Addr).toLatin1().toHex();
            I2CAddrTextBox->append(string);
        }
        else
        {
            string = "Error: " + QString(Mcp2210_GetErrorName(flag));
        }
    }
}


int main(int argc, char *argv[])
{
    //qputenv("QT_SCALE_FACTOR", "1");    //for scalability - optional
    QApplication a(argc, argv);
    MainWindow w;
    w.resize(700, 500);
    w.setWindowTitle("MCP2221 Demonstration Console");   //Window title
    a.setStyle("fusion");
    w.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);    //create flexbiel window

    Mcp2221_config();

    // write GPIO settings to MCP2221 memory
    QPushButton *applySettingsBtn = new QPushButton(&w);
    applySettingsBtn->setText("Apply");
    applySettingsBtn->setGeometry(590, 50, 100, 25);

    // DAC reference voltage (Vref) drop-down list
    QLabel *labelDacVref = new QLabel(&w);
    labelDacVref->setGeometry(505, 25, 75, 25);
    labelDacVref->setText("DAC Vref");

    DacVrefDropList = new QComboBox(&w);
    DacVrefDropList->setGeometry(505, 50, 75, 25);
    DacVrefDropList->addItems(QStringList() << "VREF_VDD" << "VREF_1024V" << "VREF_2048V" << "VREF_4096V");
    DacVrefDropList->setCurrentText("VDD");

    // ADC reference voltage (Vref) drop-down list
    QLabel *labelAdcVref = new QLabel(&w);
    labelAdcVref->setGeometry(420, 25, 75, 25);
    labelAdcVref->setText("ADC Vref");

    AdcVrefDropList = new QComboBox(&w);
    AdcVrefDropList->setGeometry(420, 50, 75, 25);
    AdcVrefDropList->addItems(QStringList() << "VREF_VDD" << "VREF_1024V" << "VREF_2048V" << "VREF_4096V");//addItems(QStringList() << "VDD" << "1.024 V" << "2.048 V" << "4.096 V");
    AdcVrefDropList->setCurrentText("VDD");

    // Pins I/O direction
    QLabel *labelPinDir = new QLabel(&w);
    labelPinDir->setGeometry(260, 25, 100, 25);
    labelPinDir->setText("Pin Direction");

    PinDirDropList = new QComboBox(&w);
    PinDirDropList->setGeometry(260, 50, 150, 25);
    PinDirDropList->addItems(QStringList() << "MCP2221_GPDIR_INPUT" << "MCP2221_GPDIR_OUTPUT");
    PinDirDropList->setCurrentText("MCP2221_GPDIR_OUTPUT");

    // GPIO function drop-down list
    QLabel *labelGpioFunc = new QLabel(&w);
    labelGpioFunc->setGeometry(100, 25, 100, 25);
    labelGpioFunc->setText("GPIO Function");

    GpioFunctionDropList = new QComboBox(&w);
    GpioFunctionDropList->setGeometry(100, 50, 150, 25);
    GpioFunctionDropList->addItems(QStringList() << "MCP2221_GPFUNC_IO" << "MCP2221_GP_ADC" << "MCP2221_GP_DAC");
    GpioFunctionDropList->setCurrentText("MCP2221_GPFUNC_IO");

    // List of pins
    QLabel *labelPinList = new QLabel(&w);
    labelPinList->setGeometry(15, 25, 75, 25);//(505, 25, 75, 25);
    labelPinList->setText("Pin");

    pinDropList = new QComboBox(&w);
    pinDropList->setGeometry(15, 50, 75, 25);//(505, 50, 75, 25);
    pinDropList->addItems(QStringList() << "GP0" << "GP1" << "GP2" << "GP3");
    pinDropList->setCurrentText("GP0");

    // checkbox for digital output function
    QSignalMapper *CheckBoxSignalMapper = new QSignalMapper(&w);
    GPIO0chkBx = new QCheckBox("GPIO0",&w);
    GPIO0chkBx->setGeometry(15, 85, 100, 20);
    GPIO1chkBx = new QCheckBox("GPIO1",&w);
    GPIO1chkBx->setGeometry(15, 115, 100, 20);
    GPIO2chkBx = new QCheckBox("GPIO2",&w);
    GPIO2chkBx->setGeometry(15, 145, 100, 20);
    GPIO3chkBx = new QCheckBox("GPIO3",&w);
    GPIO3chkBx->setGeometry(15, 175, 100, 20);

    //Serial port list
    SerialPortDropList = new QComboBox(&w);
    SerialPortDropList->setGeometry(15, 205, 75, 25);

    BaudRateDropList = new QComboBox(&w);
    BaudRateDropList->setGeometry(15, 240, 75, 25);
    BaudRateDropList->addItems(QStringList() << "4800" << "9600" << "14400" << "19200" << "38400" << "57600" << "115200" << "128000" << "256000" << "921600");
    BaudRateDropList->setCurrentText("9600");

    //Send serial data push button
    QPushButton *ConnectSerial = new QPushButton(&w);
    ConnectSerial->setText("Connect");
    ConnectSerial->setGeometry(15, 275, 75, 25);

    // textbox for serial data transmittted
    TextBox = new QTextEdit(&w);
    TextBox->setGeometry(15, 310, 75, 25);
    TextBox->acceptRichText();
    TextBox->setAcceptRichText(true);

    //Send serial data push button
    QPushButton *SendButton = new QPushButton(&w);
    SendButton->setText("Send");
    SendButton->setGeometry(15, 345, 75, 25);

    // textbox for serial data received
    ReadSerialTextBox = new QTextEdit(&w);
    ReadSerialTextBox->setGeometry(15, 380, 75, 25);
    ReadSerialTextBox->acceptRichText();
    ReadSerialTextBox->setAcceptRichText(false);

    //Serial port configuration
    serial = new QSerialPort(&w);

    //List all serial ports in the system
    QList<QSerialPortInfo> serialportlist;
    serialportlist = QSerialPortInfo::availablePorts();
    for(const QSerialPortInfo &info : serialportlist)
    {
        /*qDebug() << info.portName();
          qDebug() << info.description();
          qDebug() << info.productIdentifier();
          qDebug() << info.manufacturer();
          qDebug() << info.serialNumber();*/
        SerialPortDropList->addItem(info.portName());
    }

    // Push-button to start scanning I2C for connected slave devices
    QPushButton *ScanI2cBus = new QPushButton(&w);
    ScanI2cBus->setText("Scan I2C Bus");
    ScanI2cBus->setGeometry(15, 415, 75, 25);

    // Textbox for scanned I2C addresses on the bus
    I2CAddrTextBox = new QTextEdit(&w);
    I2CAddrTextBox->setGeometry(15, 450, 75, 25);

    timer = new QTimer(&w);
    timer->setInterval(0);
    //timer->start();
    QObject::connect(timer, SIGNAL(timeout()), &w, SLOT(TimerCallback()));

    //QChart plot configuration
    //Create line series for X,Y,Z
    seriesX = new QLineSeries();
    seriesX->setName("ADC1");
    seriesX->setColor(QColor("Red"));

    seriesY = new QLineSeries();
    seriesY->setName("ADC2");
    seriesY->setColor(QColor("Blue"));

    seriesZ = new QLineSeries();
    seriesZ->setName("ADC3");
    seriesZ->setColor(QColor("Green"));

    // Real-time plotting Chart
    chartAcc = new QChart();
    chartAcc->legend()->show();

    // Chart axes
    QValueAxis *axisX1 = new QValueAxis;
    axisX1->setRange(x1RangeMin, x1RangeMax);
    axisX1->setTitleText("Time");

    QValueAxis *axisY1 = new QValueAxis;
    axisY1->setRange(y1RangeMin, y1RangeMax);
    axisY1->setTitleText("Voltage");

    chartAcc->addAxis(axisX1, Qt::AlignBottom);
    chartAcc->addAxis(axisY1, Qt::AlignLeft);
    chartAcc->addSeries(seriesX);
    chartAcc->addSeries(seriesY);
    chartAcc->addSeries(seriesZ);

    seriesX->attachAxis(axisX1);
    seriesX->attachAxis(axisY1);
    seriesX->setUseOpenGL(true);

    seriesY->attachAxis(axisX1);
    seriesY->attachAxis(axisY1);
    seriesY->setUseOpenGL(true);

    seriesZ->attachAxis(axisX1);
    seriesZ->attachAxis(axisY1);
    seriesZ->setUseOpenGL(true);

    chartAcc->setTitle("MCP2221 Analog Readings");
    chartAcc->setPlotAreaBackgroundVisible(true);
    chartAcc->setBackgroundVisible(false);
    chartAcc->setAnimationOptions(QChart::AllAnimations);
    chartAcc->legend()->show();//hide();

    //Create Accelerometer Chart View Widget
    chartViewAcc = new QChartView(chartAcc, &w);
    chartViewAcc->setGeometry(100, 75, 600, 425);
    chartViewAcc->setRenderHint(QPainter::Antialiasing);

    w.show();
    QObject::connect(&a, SIGNAL(aboutToQuit()), &w, SLOT(ExitApp()));
    QObject::connect(applySettingsBtn, SIGNAL(clicked()), &w, SLOT(ApplySettingsCallback()));
    QObject::connect(CheckBoxSignalMapper, SIGNAL(mappedInt(int)), &w, SLOT(DigitalOutCallback(int)));
    CheckBoxSignalMapper->setMapping(GPIO0chkBx, 0);
    CheckBoxSignalMapper->setMapping(GPIO1chkBx, 1);
    CheckBoxSignalMapper->setMapping(GPIO2chkBx, 2);
    CheckBoxSignalMapper->setMapping(GPIO3chkBx, 3);
    QObject::connect(GPIO0chkBx, SIGNAL(stateChanged(int)), CheckBoxSignalMapper, SLOT(map()));
    QObject::connect(GPIO1chkBx, SIGNAL(stateChanged(int)), CheckBoxSignalMapper, SLOT(map()));
    QObject::connect(GPIO2chkBx, SIGNAL(stateChanged(int)), CheckBoxSignalMapper, SLOT(map()));
    QObject::connect(GPIO3chkBx, SIGNAL(stateChanged(int)), CheckBoxSignalMapper, SLOT(map()));
    QObject::connect(serial, SIGNAL(readyRead()), &w, SLOT(ReadSerialPort()));
    QObject::connect(SendButton, SIGNAL(clicked()), &w, SLOT(SendSerial()));
    QObject::connect(ConnectSerial, SIGNAL(clicked()), &w, SLOT(ConnectSerialCallback()));
    QObject::connect(ScanI2cBus, SIGNAL(clicked()), &w, SLOT(Mcp2221_ScanI2cBus()));

    return a.exec();
}
