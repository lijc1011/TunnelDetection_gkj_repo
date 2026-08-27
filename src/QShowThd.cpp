//#include "stdafx.h"
#include "QShowThd.h"

QShowThd::QShowThd(QObject *parent)
	: QObject(parent)
{
	for (int i = 0; i < MAXCAMNUM; ++i)
	{
		m_cam[i] = nullptr;
		pDataBuffer[i] = nullptr;
		pDataBufferSize[i] = 0;
		m_currentExposure[i] = 0.0;
		m_currentGain[i] = 0.0;
		m_manualExposureBeforeAuto[i] = 0.0;
		m_manualGainBeforeAuto[i] = 0.0;
		m_autoMin[i] = 75.0;
		m_autoMax[i] = 175.0;
		m_aeCounter[i] = 0;
		m_aeInterval[i] = 1;
		m_aeFailureCount[i] = 0;
		m_aeHoldLogCounter[i] = 0;
		m_autoReady[i] = false;
		m_autoOverrideActive[i] = false;
		m_manualStateSaved[i] = false;
	}
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
		m_autoMin[i] = std::clamp(m_camInfor[i].AutoBrightnessMin, 1.0, 253.0);
		m_autoMax[i] = std::clamp(m_camInfor[i].AutoBrightnessMax,
			m_autoMin[i] + 1.0, 254.0);
		m_aeInterval[i] = std::clamp(m_camInfor[i].AutoBrightnessInterval, 1, 30);
		m_aec[i].configure(m_autoMin[i], m_autoMax[i],
			m_camInfor[i].AutoBrightnessMaxStepRatio,
			m_camInfor[i].AutoBrightnessTrimPercent);

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

		// 模式边沿不依赖图像队列。即使相机暂时没有新帧，取消自动后也会立即
		// 恢复进入自动前的手动曝光/增益以及相机内部AE/AG状态。
		const bool autoEnabled = g_autoBrightnessEnabled.load();
		if (autoEnabled && !m_prevAutoEnabled)
			enterAutoBrightnessMode();
		else if (!autoEnabled && m_prevAutoEnabled)
			leaveAutoBrightnessMode();

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

			// 2.5 自动亮度——严格由总开关门控。手动模式不测亮度、不下发参数。
			if (autoEnabled)
				updateAutoBrightness();

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
	if (m_prevAutoEnabled)
		leaveAutoBrightnessMode();
}

void QShowThd::enterAutoBrightnessMode()
{
	m_prevAutoEnabled = true;
	for (int i = 0; i < g_program_setInfor.CameraNumber; ++i)
	{
		m_autoReady[i] = false;
		m_autoOverrideActive[i] = false;
		m_manualStateSaved[i] = false;
		m_aeCounter[i] = 0;
		m_aeFailureCount[i] = 0;
		m_aeHoldLogCounter[i] = 0;
		m_aec[i].reset();
		if (m_cam[i] == nullptr)
			continue;

		m_autoOverrideActive[i] = m_cam[i]->BeginSoftwareAutoBrightness();
		if (!m_autoOverrideActive[i])
		{
			qWarning() << m_camInfor[i].Name
				<< QStringLiteral("自动亮度启动失败：无法关闭相机内部自动曝光/增益");
			continue;
		}
		double exposure = 0.0;
		double gain = 0.0;
		const bool exposureOk = m_cam[i]->GetExpTime(exposure);
		const bool gainOk = m_cam[i]->GetCamGain(gain);
		if (!exposureOk || !gainOk || exposure <= 0.0 ||
			!std::isfinite(exposure) || !std::isfinite(gain))
		{
			qWarning() << m_camInfor[i].Name
				<< QStringLiteral("自动亮度启动失败：无法读取有效曝光/增益");
			continue;
		}

		m_currentExposure[i] = exposure;
		m_currentGain[i] = gain;
		m_manualExposureBeforeAuto[i] = exposure;
		m_manualGainBeforeAuto[i] = gain;
		m_manualStateSaved[i] = true;
		m_autoReady[i] = true;
		qInfo() << m_camInfor[i].Name << QStringLiteral("自动亮度已启动")
			<< QStringLiteral("允许范围") << m_autoMin[i] << "~" << m_autoMax[i]
			<< QStringLiteral("曝光") << exposure
			<< QStringLiteral("增益") << gain;
	}
}

void QShowThd::leaveAutoBrightnessMode()
{
	for (int i = 0; i < g_program_setInfor.CameraNumber; ++i)
	{
		if (m_cam[i] == nullptr)
			continue;

		// 先恢复进入自动前的手动值，再恢复相机内部AE/AG模式，保证切回手动后
		// 与开启自动之前一致。失败仅记录，不修改Config中的手动参数。
		if (m_manualStateSaved[i])
		{
			const bool exposureOk = m_cam[i]->SetExpTime(m_manualExposureBeforeAuto[i]);
			const bool gainOk = m_cam[i]->SetCamGain(m_manualGainBeforeAuto[i]);
			if (!exposureOk || !gainOk)
				qWarning() << m_camInfor[i].Name << QStringLiteral("退出自动亮度时恢复手动参数失败");
		}
		if (m_autoOverrideActive[i])
			m_cam[i]->EndSoftwareAutoBrightness();

		m_autoReady[i] = false;
		m_autoOverrideActive[i] = false;
		m_manualStateSaved[i] = false;
		m_aec[i].reset();
	}
	m_prevAutoEnabled = false;
	qInfo() << QStringLiteral("自动亮度已关闭，已恢复进入自动前的手动参数");
}

void QShowThd::updateAutoBrightness()
{
	for (int i = 0; i < g_program_setInfor.CameraNumber; ++i)
	{
		if (!m_autoReady[i] || m_cam[i] == nullptr)
			continue;
		if (++m_aeCounter[i] < m_aeInterval[i])
			continue;
		m_aeCounter[i] = 0;

		const int channels = m_camInfor[i].isColorCamera == 1 ? 3 : 1;
		const AutoExposureAdjustment adjustment = m_aec[i].adjust(
			pDataBuffer[i], m_camInfor[i].PictureHeight, m_camInfor[i].ImageWidth, channels,
			m_currentExposure[i], m_currentGain[i],
			m_camInfor[i].ExposureTimeMin, m_camInfor[i].ExposureTimeMax,
			m_camInfor[i].GainMin, m_camInfor[i].GainMax);
		if (!adjustment.valid)
		{
			qWarning() << m_camInfor[i].Name << QStringLiteral("自动亮度跳过：亮度或参数无效");
			continue;
		}

		bool setOk = true;
		if (adjustment.changed)
		{
			const bool exposureChanged = std::abs(adjustment.exposure - m_currentExposure[i]) > 0.0001;
			const bool gainChanged = std::abs(adjustment.gain - m_currentGain[i]) > 0.0001;
			// 变暗时先降增益；增亮时先加曝光。两种情况下都减少瞬时过曝风险。
			if (adjustment.gain < m_currentGain[i])
			{
				if (gainChanged) setOk = m_cam[i]->SetCamGain(adjustment.gain) && setOk;
				if (exposureChanged) setOk = m_cam[i]->SetExpTime(adjustment.exposure) && setOk;
			}
			else
			{
				if (exposureChanged) setOk = m_cam[i]->SetExpTime(adjustment.exposure) && setOk;
				if (gainChanged) setOk = m_cam[i]->SetCamGain(adjustment.gain) && setOk;
			}
		}

		double actualExposure = m_currentExposure[i];
		double actualGain = m_currentGain[i];
		const bool readOk = m_cam[i]->GetExpTime(actualExposure) &&
			m_cam[i]->GetCamGain(actualGain);
		if (setOk && readOk)
		{
			m_currentExposure[i] = actualExposure;
			m_currentGain[i] = actualGain;
			m_aeFailureCount[i] = 0;
		}
		else if (++m_aeFailureCount[i] >= 3)
		{
			m_autoReady[i] = false;
			if (m_manualStateSaved[i])
			{
				m_cam[i]->SetExpTime(m_manualExposureBeforeAuto[i]);
				m_cam[i]->SetCamGain(m_manualGainBeforeAuto[i]);
				m_manualStateSaved[i] = false;
			}
			if (m_autoOverrideActive[i])
			{
				m_cam[i]->EndSoftwareAutoBrightness();
				m_autoOverrideActive[i] = false;
			}
			qCritical() << m_camInfor[i].Name
				<< QStringLiteral("自动亮度已停用：连续3次下发或回读失败；其它相机继续工作");
		}

		if (adjustment.changed || ++m_aeHoldLogCounter[i] >= 10)
		{
			m_aeHoldLogCounter[i] = 0;
			qInfo() << m_camInfor[i].Name
				<< QStringLiteral("AE亮度/范围") << adjustment.brightness
				<< "/" << m_autoMin[i] << "~" << m_autoMax[i]
				<< QStringLiteral("曝光") << actualExposure
				<< QStringLiteral("增益") << actualGain
				<< QStringLiteral("下发回读") << (setOk && readOk ? QStringLiteral("正常") : QStringLiteral("失败"));
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
