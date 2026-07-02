//#include "stdafx.h"
#include "QShowThd.h"

QShowThd::QShowThd(QObject *parent)
	: QObject(parent)
{
 
}

QShowThd::~QShowThd()
{

}
void QShowThd::setCamera(ICamera *pDel[], CAMERAINFORSTRUCT camInfor[])
{
	for (int i = 0; i < g_program_setInfor.CameraNumber; i++)
	{
		m_cam[i] = pDel[i];
		m_camInfor[i] = camInfor[i];

		pDataBufferSize[i] = MAXFRAME*m_camInfor[i].iSizeFrame;
		pDataBuffer[i] = new BYTE[pDataBufferSize[i]];
		memset((void *)pDataBuffer[i], 0, pDataBufferSize[i]);
	}

}
void QShowThd::setCamera(QLabel* plabel[])
{
	for (int i = 0; i < g_program_setInfor.CameraNumber; i++)
	{
		m_labelShow[i] = plabel[i];
	}
}
void QShowThd::startThd()
{
	m_isBreakThd_org = true;
	m_isCanRun_org = true;
	std::thread *pt1(new std::thread(genOrgJpgFun, this));
}
void QShowThd::stopThd()
{
	m_isBreakThd_org = false;
	m_isCanRun_org = false;
}
void QShowThd::readThd(__int64 readPos, __int64 readH)
{
	currentTime_S = QDateTime::currentDateTime().toMSecsSinceEpoch();
	m_isCanRun_org = true;
}
void QShowThd::pauseThd()
{
	m_isCanRun_org = false;
}
void QShowThd::genOrgJpgFun(QShowThd* pParameter)
{
	pParameter->sub_genOrgJpgFun();
}
void QShowThd::sub_genOrgJpgFun()
{
	while (m_isBreakThd_org)
	{
		std::lock_guard<std::mutex> lockGuard(my_lock_org);
		std::this_thread::sleep_for(std::chrono::milliseconds(g_program_setInfor.ShowThdTime));

		if (m_isCanRun_org)
		{
			// 1.判断4个采集队列的数据都超过“4096图片”，则开始刷新显示
			int isum = 0;
			for (int i = 0; i < g_program_setInfor.CameraNumber; i++)
			{
				if (m_cam[i]->IsShowII())
				{
					isum += 1;
 				}
 			}
			if (isum < g_program_setInfor.CameraNumber)
			{
				continue;
			}

			// 2.顺序读取4个队列，放到4个缓存区域
			for (int i = 0; i < g_program_setInfor.CameraNumber; i++)
			{
				m_cam[i]->GetShowDeqDataII(pDataBuffer[i], MAXFRAME);
			}

			// 2.5 自动亮度(AE)——总开关门控。
			// g_autoBrightnessEnabled==false(手动)时整段不进，显示/存储与原程序逐字节一致。
			if (g_autoBrightnessEnabled.load())
			{
				// 手动->自动 上升沿：用相机当前真实曝光/增益播种并清积分(B1/A4)
				if (!m_prevAutoEnabled)
				{
					for (int i = 0; i < g_program_setInfor.CameraNumber; i++)
					{
						double e = 0.0, g = 0.0;
						if (m_cam[i]->GetExpTime(e)) m_currentExposure[i] = static_cast<float>(e);
						if (m_cam[i]->GetCamGain(g)) m_currentGain[i] = static_cast<float>(g);
						m_aec[i].reset();
					}
					m_aeCounter = 0;
				}
				m_prevAutoEnabled = true;

				// 降频执行：每拼好 N 幅图才跑一次，给相机反应时间，防振荡(B3)
				if (++m_aeCounter >= m_aeInterval)
				{
					m_aeCounter = 0;
					for (int i = 0; i < g_program_setInfor.CameraNumber; i++)
					{
						// 入参顺序修正为(height,width)(B2)：整幅图 高=PictureHeight,宽=ImageWidth
						m_aec[i].adjust(pDataBuffer[i],
							m_camInfor[i].PictureHeight,
							m_camInfor[i].ImageWidth,
							m_currentExposure[i],
							m_currentGain[i],
							g_cameraInfor[i].ExposureTimeMin,
							g_cameraInfor[i].ExposureTimeMax,
							g_cameraInfor[i].GainMin,
							g_cameraInfor[i].GainMax);
						m_cam[i]->SetExpTime(m_currentExposure[i]);
						m_cam[i]->SetCamGain(static_cast<int>(m_currentGain[i] + 0.5f));
					}
				}
			}
			else
			{
				m_prevAutoEnabled = false;
			}

			// 3.对应在4个窗口显示
			for (int i = 0; i < g_program_setInfor.CameraNumber; i++)
			{
// 				QString imgName = QString("cam-%2.jpg").arg(i + 1, 2, 10, QChar('0'));
// 				Capture(pDataBuffer[i], m_camInfor[i].ImageWidth, m_camInfor[i].PictureHeight, imgName);
 
				//刷新显示
				//图像宽高通过参数文件设置
				if (m_camInfor[i].isColorCamera == 0)//黑白
				{
					QImage picQImage = QImage(pDataBuffer[i], m_camInfor[i].ImageWidth, m_camInfor[i].PictureHeight, QImage::Format_Indexed8);
					QPixmap picQPixmap = QPixmap::fromImage(picQImage);
					emit threadover_Camdat_std(picQPixmap, i);
				}
				else if (m_camInfor[i].isColorCamera == 1) // 彩色
				{
					QImage picQImage = QImage(pDataBuffer[i], m_camInfor[i].ImageWidth, m_camInfor[i].PictureHeight, QImage::Format_RGB888);
					QPixmap picQPixmap = QPixmap::fromImage(picQImage);
					emit threadover_Camdat_std(picQPixmap, i);
				}

			


			}

			
		
			
			currentTime_E = QDateTime::currentDateTime().toMSecsSinceEpoch();
			long long abc1 = abs(currentTime_E - currentTime_S);

		}
	}
}
void QShowThd::Capture(BYTE* pBits, int w, int h, QString imgName)
{
	// 准备路径
	QString acq_img;
	acq_img += "D:\\";
	acq_img += imgName;

	vector<int>compression_params;
	compression_params.push_back(cv::IMWRITE_JPEG_QUALITY); // PNG格式图片的压缩级别
	compression_params.push_back(95);

	 
	char ImgName_c[256];
	strcpy(ImgName_c, acq_img.toLocal8Bit().data());
 	Mat mat(h, w, CV_8U, pBits);
	imwrite(ImgName_c, mat, compression_params);

 
}