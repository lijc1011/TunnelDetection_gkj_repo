#include "TunnelDetection.h"



int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
#if 0
    std::cout << "Current path is: " << std::filesystem::current_path() << std::endl;

    // get handle to a File on the filesystem
    fs::path exeDir = fs::absolute(argv[0]).parent_path();
    z5::filesystem::handle::File f(exeDir / "data.zr");

    // if you wanted to use a different backend, for example AWS, you
    // would need to use this instead:
    // z5::s3::handle::File f;

    // create the file in zarr format
    const bool createAsZarr = true;
    if (fs::exists(exeDir / "data.zr"))
    {
        std::cout << "data.zr already exists, skip creating." << std::endl;
    }
    else
    {
        z5::createFile(f, createAsZarr);

        // create a new zarr dataset
        const std::string dsName = "data";
        std::vector<size_t> shape = {1000, 1000, 1000};
        std::vector<size_t> chunks = {100, 100, 100};
        auto ds = z5::createDataset(f, dsName, "float32", shape, chunks);

        // write array to roi
        z5::types::ShapeType offset1 = {50, 100, 150};
        xt::xarray<float>::shape_type shape1 = {150, 200, 100};
        xt::xarray<float> array1(shape1, 42.0);
        z5::multiarray::writeSubarray<float>(ds, array1, offset1.begin());

        // read array from roi (values that were not written before are filled with a fill-value)
        z5::types::ShapeType offset2 = {100, 100, 100};
        xt::xarray<float>::shape_type shape2 = {300, 200, 75};
        xt::xarray<float> array2(shape2);
        z5::multiarray::readSubarray<float>(ds, array2, offset2.begin());

        // get handle for the dataset
        const auto dsHandle = z5::filesystem::handle::Dataset(f, dsName);

        // read and write json attributes
        nlohmann::json attributesIn;
        attributesIn["bar"] = "foo";
        attributesIn["pi"] = 3.141593;
        z5::writeAttributes(dsHandle, attributesIn);

        nlohmann::json attributesOut;
        z5::readAttributes(dsHandle, attributesOut);
    }

    // ================== 【新增】OpenCV 验证代码 ==================
    // 1. 验证版本号打印
    qDebug() << "========================================";
    qDebug() << "成功链接！当前 OpenCV 版本为:" << CV_VERSION;
    qDebug() << "========================================";

    // 2. 验证核心矩阵功能（创建一个 100x100 的全零矩阵）
    cv::Mat testMat = cv::Mat::zeros(100, 100, CV_8UC1);
    if (!testMat.empty())
    {
        qDebug() << "OpenCV 内存矩阵创建成功，尺寸为:" << testMat.cols << "x" << testMat.rows;
    }
    // ============================================================



    // 验证相机sdk 是否成功链接
    SapAcqDevice *m_AcqDevice;
    SapBuffer *m_Buffers;
    SapTransfer *m_Xfer;
    //////////////////////////////////////////////////////////////////////////
    SapManager *m_pManger;
    SapAcquisition *m_pAcquisition;
    //	SapAcqDevice     *m_pCamera_m_AcqDevice;
    //	SapBuffer        *m_pBuffers;
    SapBuffer *m_pBufferExtrack;
    //	SapTransfer      *m_Xfer;
    SapFeature *m_pFeature;

    int serverCount = 0;
    SapManager::SetDisplayStatusMode(SapManager::StatusCustom); // Gets/sets the global reporting mode for messages and errors.
    serverCount = SapManager::GetServerCount();                 // Gets the number of available Sapera servers.

    int GenieIndex = 0;
    BOOL serverFound = FALSE;
    char serverName[CORSERVER_MAX_STRLEN];
    if (serverCount == 1)
    {
        return false;
    }
    for (int serverIndex = 0; serverIndex < serverCount; serverIndex++)
    {
        // Gets the number of resources of a specified type on a Sapera server.
        char userDefinedName[100] = {0};
        if (SapManager::GetResourceCount(serverIndex, SapManager::ResourceAcqDevice) != 0)
        {
            // Gets the name of a Sapera resource of a specified type.
            if (SapManager::GetResourceName(serverIndex, SapManager::ResourceAcqDevice, serverName, sizeof(serverName)))
            {

                //	if (strcmp(serverName, lpPath) == 0)
                //	{
                //		SapManager::GetServerName(serverIndex, serverName, sizeof(serverName)); // Gets the name of a Sapera server.
                //		m_AcqDevice = new SapAcqDevice(serverName);
                //		return true;
                //	}
            }
        }
    }
    m_Buffers = new SapBufferWithTrash(10, m_AcqDevice);
    // m_Xfer = new SapAcqDeviceToBuf(m_AcqDevice, m_Buffers, XferCallback, this);

    // Create acquisition object
    if (m_AcqDevice && !*m_AcqDevice && !m_AcqDevice->Create())
    {
        //	WRITE_LOG("CreateObjects1",LOG_LEVEL_RUN);
        //	DestroyObjects();
        return FALSE;
    }

    // Create buffer object
    if (m_Buffers && !*m_Buffers)
    {
        if (!m_Buffers->Create())
        {
            //	WRITE_LOG(_T("CreateObjects2"),LOG_LEVEL_RUN);
            //	DestroyObjects();
            return FALSE;
        }
        // Clear all buffers
        m_Buffers->Clear();
    }

    // Create view object
    // 	if (m_View && !*m_View && !m_View->Create())
    // 	{
    // 		WRITE_LOG(_T("CreateObjects3"),LOG_LEVEL_RUN);
    // 		DestroyObjects();
    // 		return FALSE;
    // 	}
    //	//图像适应现实控件大小
    //	m_View->SetScalingMode(SapView::ScalingFitToWindow);

    // Set next empty with trash cycle mode for transfer
    if (m_Xfer && m_Xfer->GetPair(0))
    {
        if (!m_Xfer->GetPair(0)->SetCycleMode(SapXferPair::CycleNextWithTrash))
        {
            //	WRITE_LOG(_T("CreateObjects4"),LOG_LEVEL_RUN);
            //	DestroyObjects();
            return FALSE;
        }
    }

    // Create transfer object
    if (m_Xfer && !*m_Xfer && !m_Xfer->Create())
    {
        //	WRITE_LOG(_T("CreateObjects5"),LOG_LEVEL_RUN);
        //	DestroyObjects();
        return FALSE;
    }

#endif
 
    TunnelDetection w;
    w.setGeometry(1920, 0, 1920, 1080);
    w.showMaximized();
    w.show();

    return a.exec();
}