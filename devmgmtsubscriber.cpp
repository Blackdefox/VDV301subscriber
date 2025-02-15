#include "devmgmtsubscriber.h"


DevMgmtSubscriber::DevMgmtSubscriber(QString nazevSluzby, QString struktura, QString  verze, QString  typSluzby, int cisloPortu) : IbisIpSubscriber(nazevSluzby,  struktura,  verze,  typSluzby, cisloPortu)
{
    qDebug()<<Q_FUNC_INFO;
    vsechnyConnecty();
    hledejSluzby(mTypSluzby,1);
}

DevMgmtSubscriber::~DevMgmtSubscriber()
{
    qDebug()<<Q_FUNC_INFO;
    zeroConf.disconnect();//reseni erroru ASSERT failure in DevMgmtSubscriber: "Called object is not of the correct type
    //  this->disconnect();
}

void DevMgmtSubscriber::vsechnyConnecty()
{
    qDebug()<<Q_FUNC_INFO;
    connect(&zeroConf, &QZeroConf::serviceAdded, this, &DevMgmtSubscriber::slotNewDnsSd);
    connect(&zeroConf, &QZeroConf::serviceRemoved, this, &DevMgmtSubscriber::slotRemoveDnsSd);

    connect(&manager,&QNetworkAccessManager::finished,this,&DevMgmtSubscriber::slotRequestReceived);
    connect(this,&DevMgmtSubscriber::stazeniHotovo,this,&DevMgmtSubscriber::slotZpracujData);

}


bool DevMgmtSubscriber::getDeviceInformation(QZeroConfService zcs)
{

    qDebug()<<Q_FUNC_INFO;

    //   QString adresa=zarizeni.adresa.toString()+":"+QString::number(zarizeni.port)+
    QString adresa = "http://"+zcs->ip().toString()+":"+QString::number(zcs->port())+"/DeviceManagementService/GetDeviceInformation";
    qDebug()<<adresa;
    QNetworkRequest pozadavek;
    pozadavek.setUrl(QUrl(adresa));

    manager.get(pozadavek);

    return true;
}


bool DevMgmtSubscriber::getDeviceInformation(DevMgmtPublisherStruct &zarizeni)
{

    qDebug()<<Q_FUNC_INFO;

    //   QString adresa=zarizeni.adresa.toString()+":"+QString::number(zarizeni.port)+
    QString adresa = "http://"+zarizeni.adresa.toString()+":"+QString::number(zarizeni.port)+"/DeviceManagementService/GetDeviceInformation";
    qDebug()<<adresa;
    QNetworkRequest pozadavek;
    pozadavek.setUrl(QUrl(adresa));

    manager.get(pozadavek);

    return true;
}


bool DevMgmtSubscriber::getDeviceConfiguration(QZeroConfService zcs)
{
    qDebug()<<Q_FUNC_INFO;

    //   QString adresa=zarizeni.adresa.toString()+":"+QString::number(zarizeni.port)+
    QString adresa = "http://"+zcs->ip().toString()+":"+QString::number(zcs->port())+"/DeviceManagementService/GetDeviceConfiguration";
    qDebug()<<adresa;
    manager.get(QNetworkRequest(QUrl(adresa)));

    return true;
}

bool DevMgmtSubscriber::getDeviceConfiguration(DevMgmtPublisherStruct &zarizeni)
{
    qDebug()<<Q_FUNC_INFO;

    //   QString adresa=zarizeni.adresa.toString()+":"+QString::number(zarizeni.port)+
    QString adresa = "http://"+zarizeni.adresa.toString()+":"+QString::number(zarizeni.port)+"/DeviceManagementService/GetDeviceConfiguration";
    qDebug()<<adresa;
    manager.get(QNetworkRequest(QUrl(adresa)));

    return true;
}

QByteArray DevMgmtSubscriber::slotRequestReceived(QNetworkReply* reply)
{
    qDebug() <<  Q_FUNC_INFO;
    QByteArray rawData = reply->readAll();
    qDebug()<<"Tohle je reply Url: "<<reply->request().url();


    QString textData(rawData);
    //  qDebug() << textData;

    emit stazeniHotovo(textData);


    DevMgmtPublisherStruct zarizeni;

    QDomDocument dokument;
    dokument.setContent(textData);

    //   zarizeni.adresa=QHostAddress("192.168.0.131");//reply->request().url()
    zarizeni.adresa=QHostAddress( reply->request().url().host());
    zarizeni.port=reply->request().url().port();


    int index=seznamZarizeniDetekce.indexOf(zarizeni);
    if((index>=0)&&(index<seznamZarizeniDetekce.count()))
    {

        if(dokument.firstChildElement().nodeName()=="DeviceManagementService.GetDeviceConfigurationResponse")
        {
            qDebug()<<"ano, hledam Configuration";
            zarizeni.deviceId=dokument.elementsByTagName("DeviceID").at(0).firstChildElement("Value").firstChild().nodeValue();
            seznamZarizeniDetekce[index].deviceId=zarizeni.deviceId;
            qDebug()<<"device id je: "<< zarizeni.deviceId<<" index na seznamu"<<index<<" adresa:"<<zarizeni.adresa<<" port:"<<zarizeni.port;
        }

        if(dokument.firstChildElement().nodeName()=="DeviceManagementService.GetDeviceInformationResponse")
        {
            qDebug()<<"ano, hledam Information ";

            zarizeni.deviceClass=dokument.elementsByTagName("DeviceClass").at(0).firstChild().nodeValue();
            zarizeni.deviceName=dokument.elementsByTagName("DeviceName").at(0).firstChildElement("Value").firstChild().nodeValue();
            zarizeni.manufacturer=dokument.elementsByTagName("Manufacturer").at(0).firstChildElement("Value").firstChild().nodeValue();
            zarizeni.serialNumber=dokument.elementsByTagName("SerialNumber").at(0).firstChildElement("Value").firstChild().nodeValue();
            zarizeni.swVersion=getVersion(dokument,"SwVersion");

            seznamZarizeniDetekce[index].deviceClass=zarizeni.deviceClass;
            seznamZarizeniDetekce[index].deviceName=zarizeni.deviceName;
            seznamZarizeniDetekce[index].manufacturer=zarizeni.manufacturer;
            seznamZarizeniDetekce[index].serialNumber=zarizeni.serialNumber;
            seznamZarizeniDetekce[index].swVersion=zarizeni.swVersion;
            qDebug()<<"device class je: "<< zarizeni.deviceClass<<" verze: "<<zarizeni.swVersion<<" index na seznamu"<<index<<" adresa:"<<zarizeni.adresa<<" port:"<<zarizeni.port;

        }



        if((!seznamZarizeniDetekce[index].deviceId.isEmpty())&&(!seznamZarizeniDetekce[index].deviceClass.isEmpty()))
        {
            DevMgmtPublisherStruct* noveZarizeni=&seznamZarizeniDetekce[index];

            if(noveZarizeni->isInListByIdClass(seznamZarizeniKonfigurace))
            {
                noveZarizeni->hwConfig=true;
            }




        }


    }
    else
    {
        qDebug()<<"index je mimo rozsah";
    }

    emit signalAktualizaceSeznamu();


    return rawData;
}

void DevMgmtSubscriber::slotZpracujData(QString vstup)
{
    qDebug()<<Q_FUNC_INFO;
    qDebug().noquote()<<vstup;
}


QString DevMgmtSubscriber::getVersion(QDomDocument document, QString element)
{
    QString vysledek="";
    QDomNodeList seznamElementu = document.elementsByTagName("DataVersion");

    for (int i=0;i<seznamElementu.count();i++)
    {
        QString tagName=seznamElementu.at(i).toElement().firstChildElement("DataType").firstChildElement("Value").firstChild().nodeValue();
        if(tagName==element)
        {
            vysledek=seznamElementu.at(i).toElement().firstChildElement("VersionRef").firstChildElement("Value").firstChild().nodeValue();
            return vysledek;
        }


    }

    return vysledek;
}



void DevMgmtSubscriber::slotNewDnsSd(QZeroConfService zcs)
{
    qDebug() <<  Q_FUNC_INFO;

    DevMgmtPublisherStruct noveZarizeni;
    noveZarizeni.serviceName=zcs->name();



    if(noveZarizeni.serviceName.contains("DeviceManagementService"))
    {
        qDebug()<<"DP1";
        noveZarizeni.adresa=zcs->ip();
        noveZarizeni.port=zcs->port();
        noveZarizeni.deviceClass="";
        noveZarizeni.deviceId="";
        noveZarizeni.hostname=zcs->host();
        noveZarizeni.ibisIpVersion=zcs.data()->txt().value("ver");



        if(!seznamZarizeniDetekce.contains(noveZarizeni))
        {
            qDebug()<<"DP2";
            getDeviceConfiguration(noveZarizeni);
            getDeviceInformation(noveZarizeni);

            qDebug()<<"DP5";
            seznamZarizeniDetekce.push_back(noveZarizeni);

        }
        else
        {
            qDebug()<<"zarizeni uz je na seznamu";
        }
    }
    else
    {
        qDebug()<<"jina sluzba";
    }

    emit signalAktualizaceSeznamu();
}

void DevMgmtSubscriber::slotAktualizaceZarizeni()
{
    qDebug() <<  Q_FUNC_INFO;
    foreach(auto polozka, seznamZarizeniDetekce)
    {
        getDeviceConfiguration(polozka);
        getDeviceInformation(polozka);

    }
    emit signalAktualizaceSeznamu();
}

void DevMgmtSubscriber::slotRemoveDnsSd(QZeroConfService zcs)
{
    qDebug() <<  Q_FUNC_INFO;

    DevMgmtPublisherStruct noveZarizeni;
    noveZarizeni.serviceName=zcs->name();

    if(noveZarizeni.serviceName.contains("DeviceManagementService"))
    {
        noveZarizeni.adresa=zcs->ip();
        noveZarizeni.port=zcs->port();
        noveZarizeni.deviceClass="";
        noveZarizeni.deviceId="";
        // noveZarizeni.deviceName=zcs->host();

        noveZarizeni.hostname=zcs->host();
        noveZarizeni.ibisIpVersion=zcs.data()->txt().value("ver");

        if(seznamZarizeniDetekce.contains(noveZarizeni))
        {
            seznamZarizeniDetekce.remove(seznamZarizeniDetekce.indexOf(noveZarizeni) );
        }
        else
        {
            qDebug()<<"zarizeni nebylo na seznamu";
        }
    }




    emit signalAktualizaceSeznamu();
}
