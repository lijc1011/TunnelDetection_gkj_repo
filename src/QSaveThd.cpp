//#include "stdafx.h"
#include "QSaveThd.h"

QSaveThd::QSaveThd(QObject *parent, CCamera *pDel, CAMERAINFORSTRUCT camInfor, int ichl)
	: QObject(parent)
{
	m_ichl = ichl;
	m_cam = pDel;
	m_camInfor = camInfor;

	m_pPicture = new PICTUREIMAGESTRUCT;
	m_pPicture->iBufferSize = MAXFRAME*m_camInfor.iSizeFrame;//camInfor.ImageWidth*camInfor.PictureHeight
	m_pPicture->pDataBuffer = new BYTE[m_pPicture->iBufferSize];
	memset((void *)m_pPicture->pDataBuffer, 0, m_pPicture->iBufferSize);
	m_pPicture->picInfor.iSizePicture = m_pPicture->iBufferSize;


	lpFile_camData = NULL;
	lpFile_camIdx = NULL;

	m_iPicCnt = 0;

	im_encode = NULL;
}

QSaveThd::~QSaveThd()
{
	stopThd();
}
void QSaveThd::startThd()
{
	m_isBreakThd_org = true;
	m_isCanRun_org = true;
	std::thread *pt1(new std::thread(genOrgJpgFun, this));
}
void QSaveThd::stopThd()
{
	m_isBreakThd_org = false;
	m_isCanRun_org = false;
}
void QSaveThd::pauseThd()
{
	m_isCanRun_org = false;
}
void QSaveThd::genOrgJpgFun(QSaveThd* pParameter)
{
	pParameter->sub_genOrgJpgFun();
}
void QSaveThd::sub_genOrgJpgFun()
{
	while (m_isBreakThd_org)
	{
		std::lock_guard<std::mutex> lockGuard(my_lock_org);
		std::this_thread::sleep_for(std::chrono::milliseconds(10));

		if (m_isCanRun_org)
		{
			// 1.是否获得开始存储的命令
			if (g_isSave[m_ichl] == false)
			{
				emit thread_signal_stopsave(m_ichl);
				continue;
			}
#if 1
			// 2.判断是否满足存储条件
			if (m_cam->IsSave() == false)
			{
				continue;
			}

			// 3.若满足条件,则从队列取图片
			m_cam->GetSaveDeqData(m_pPicture);

			// 			QString imgName = QString("cam-%2.jpg").arg(m_ichl + 1, 2, 10, QChar('0'));
			// 			Capture((BYTE *)m_pPicture->pDataBuffer, m_camInfor.ImageWidth, m_camInfor.PictureHeight, imgName);

			// 4.存文件
			SaveFrameData(m_pPicture, m_iPicCnt);
			m_iPicCnt++;
			memset((void *)m_pPicture->pDataBuffer, 0, sizeof(m_pPicture->picInfor.iSizePicture));

				emit thread_signal(m_ichl);
				// 5.释放队列
				// 因为已开始存储，所以存储完在这里释放
				//  			int isize = m_pPicture->picInfor.iFrameNum;
				//  			m_cam->ClearSaveDeg(isize);

#endif
		}
	}
}
void QSaveThd::Capture(BYTE* pBits, int w, int h, QString imgName)
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
BOOL QSaveThd::SaveFrameData(pPICTUREIMAGESTRUCT pPicImg, __int64 isn)
{
	if (lpFile_camData == NULL) return FALSE;
	if (lpFile_camIdx == NULL) return FALSE;

	// 先存camdata,pPicImg.pDataBuffer
	// 先加密再存储
	if (im_encode != NULL)
	{
		im_encode->encrypt_data((char*)pPicImg->pDataBuffer, pPicImg->iBufferSize);
	}
	int a = fwrite(pPicImg->pDataBuffer, pPicImg->iBufferSize, 1, lpFile_camData);
	// 再存camidx，pPicImg.picInfor
	// 这里写索引信息
	pPicImg->picInfor.iPicCnt = isn;
	__int64 tmp = pPicImg->picInfor.iSizePicture;
	pPicImg->picInfor.startPosition = isn*tmp;
	pPicImg->picInfor.recordDimension = pPicImg->picInfor.iSizePicture;
	int b = fwrite(&pPicImg->picInfor, sizeof(PICTUREINFORSTRUCT), 1, lpFile_camIdx);


	return TRUE;
}

void QSaveThd::StartSave(QString str_path, QString str_file)
{
	g_isSave[m_ichl] = true;

	str_path += "\\";
	str_path += str_file;

	// 打开相机数据文件
	OpenCamDataSaveFile(str_path);

	// 同时打开相机数据索引文件
	OpenCamIdxSaveFile(str_path);

	m_iPicCnt = 0; // 每次存储前，序号归零
	m_pPicture->picInfor.startPosition = 0;


}
void QSaveThd::OpenCamDataSaveFile(QString str_path_file)
{
	// 打开文件
	m_strAllfile_camData = str_path_file;

	// 取出文件名，利用时间和相机名称进行加密，例如"20250514144747-1Cam_ZQJ1"
	QFileInfo fi(m_strAllfile_camData);
	QString str_FileNameKey = fi.fileName();
	str_FileNameKey = str_FileNameKey.mid(0, str_FileNameKey.length() - 4); // 减去.tmp

	int startPos = str_FileNameKey.indexOf('-', 0);
	QString str_Key = str_FileNameKey.mid(0, startPos); // 时间
	str_Key += "-";
 
	str_FileNameKey = str_FileNameKey.mid(startPos + 1, str_FileNameKey.length() - startPos);
	startPos = str_FileNameKey.indexOf('-', 0);
	str_Key += str_FileNameKey.mid(0, startPos);//相机名称

	if (im_encode == NULL)
	{
		std::string secret_key = str_Key.toStdString();
	//	qDebug() << str_Key;
		im_encode = new ImageEncryptor(secret_key);
	}
 
	char xmlPath[RouteInfoSize];
	strcpy(xmlPath, m_strAllfile_camData.toLocal8Bit().data());

	if (fopen_s(&lpFile_camData, xmlPath, "wb") != 0)
	{
		return;
	}

}
void QSaveThd::OpenCamIdxSaveFile(QString str_path_file)
{
	QString str1 = QStringLiteral(".camdatx.tmp");
	QString strtmp = str_path_file.mid(0, str_path_file.length() - str1.length());
	m_strAllfile_camIdx = strtmp;
	m_strAllfile_camIdx += QStringLiteral(".camidx.tmp");

	QFileInfo fi(m_strAllfile_camIdx); // 取出文件名，存台账
	m_str_IdxFileName = fi.fileName();

	char xmlPath[RouteInfoSize];
	strcpy(xmlPath, m_strAllfile_camIdx.toLocal8Bit().data());

	// 打开文件
	if (fopen_s(&lpFile_camIdx, xmlPath, "wb") != 0)
	{
		return;
	}

}
void QSaveThd::StopSave()
{
	g_isSave[m_ichl] = false;
	StopSaveImg();
}
void QSaveThd::StopSaveImg(void)
{
	// 停止存储
	Sleep(100);

	// 关闭文件
	CloseSaveFile();

	// 重命名文件
	ReNameDatFile();//tFile();
	ReNameIdxFile();

	m_iPicCnt = 0;
	m_pPicture->picInfor.startPosition = 0;


}
void QSaveThd::CloseSaveFile()
{
	if (lpFile_camData != NULL)
	{
		fclose(lpFile_camData); /*关闭文件*/
		lpFile_camData = NULL;
	}

	if (lpFile_camIdx != NULL)
	{
		fclose(lpFile_camIdx); /*关闭文件*/
		lpFile_camIdx = NULL;
	}

	if (im_encode != NULL) 
	{
		delete im_encode;
		im_encode = NULL;
	}
}
void QSaveThd::ReNameDatFile(void)
{
	// 修改文件名，去掉后缀
	QString strold = m_strAllfile_camData;
	QString strNew = m_strAllfile_camData.mid(0, m_strAllfile_camData.length() - 4); // 减去.tmp

	char pOld[RouteInfoSize];
	strcpy(pOld, strold.toLocal8Bit().data());
	char pNew[RouteInfoSize];
	strcpy(pNew, strNew.toLocal8Bit().data());

	rename(pOld, pNew);

	QFileInfo fi(strNew); // 取出文件名，存台账
	m_str_ImgFileName = fi.fileName();

}
void QSaveThd::ReNameIdxFile(void)
{
	// 修改文件名，去掉后缀
	QString strold = m_strAllfile_camIdx;
	QString strNew = m_strAllfile_camIdx.mid(0, m_strAllfile_camIdx.length() - 4); // 减去.tmp

	char pOld[RouteInfoSize];
	strcpy(pOld, strold.toLocal8Bit().data());
	char pNew[RouteInfoSize];
	strcpy(pNew, strNew.toLocal8Bit().data());

	rename(pOld, pNew);

	QFileInfo fi(strNew);//取出文件名，存台账
	m_str_IdxFileName = fi.fileName();

}
