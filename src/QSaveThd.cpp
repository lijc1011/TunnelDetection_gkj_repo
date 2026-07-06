//#include "stdafx.h"
#include "QSaveThd.h"

#include <QDir>
#include <QFileInfo>

QSaveThd::QSaveThd(QObject *parent, ICamera *pDel, CAMERAINFORSTRUCT camInfor, int ichl)
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

	// Zarr v3 (TensorStore) 存储模式
	m_zarrMode = (g_program_setInfor.SaveZarr != 0);
	m_tsWriter = NULL;
	m_pTransformBuf = NULL;
	m_iPacketBytes = m_pPicture->iBufferSize;
	const int channels = (m_camInfor.isColorCamera != 0) ? 3 : 1;
	m_matType = (channels == 3) ? CV_8UC3 : CV_8UC1;
	m_matCols = m_camInfor.ImageWidth;
	m_matRows = (m_matCols > 0) ? (int)(m_iPacketBytes / ((__int64)m_matCols * channels)) : 0;
	const bool swapWH = (m_camInfor.SaveTransform == 4 || m_camInfor.SaveTransform == 5);
	m_zarrRows = swapWH ? m_matCols : m_matRows;
	m_zarrCols = (swapWH ? (__int64)m_matRows : (__int64)m_matCols) * channels;
}

QSaveThd::~QSaveThd()
{
	stopThd();
	if (m_tsWriter != NULL)
	{
		CloseZarrStore();
	}
	if (m_pTransformBuf != NULL)
	{
		delete[] m_pTransformBuf;
		m_pTransformBuf = NULL;
	}
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
BYTE* QSaveThd::TransformAndEncrypt(BYTE* pSrc)
{
	BYTE* pOut = pSrc;
	if (m_camInfor.SaveTransform != 0 && m_pTransformBuf != NULL)
	{
		Mat msrc(m_matRows, m_matCols, m_matType, pSrc);
		Mat mdst((int)m_zarrRows, (int)(m_zarrCols / ((m_matType == CV_8UC3) ? 3 : 1)),
				 m_matType, m_pTransformBuf);
		switch (m_camInfor.SaveTransform)
		{
		case 1: cv::flip(msrc, mdst, 1); break;             // 水平翻转（镜像行方向）
		case 2: cv::flip(msrc, mdst, 0); break;             // 垂直翻转（倒转行序）
		case 3: cv::rotate(msrc, mdst, cv::ROTATE_180); break;
		case 4: cv::rotate(msrc, mdst, cv::ROTATE_90_CLOCKWISE); break;
		case 5: cv::rotate(msrc, mdst, cv::ROTATE_90_COUNTERCLOCKWISE); break;
		default: break;
		}
		pOut = m_pTransformBuf;
	}
	if (im_encode != NULL)
	{
		im_encode->encrypt_data((char*)pOut, m_iPacketBytes);
	}
	return pOut;
}
BOOL QSaveThd::SaveFrameData(pPICTUREIMAGESTRUCT pPicImg, __int64 isn)
{
	if (lpFile_camIdx == NULL) return FALSE;

	if (m_zarrMode)
	{
		if (m_tsWriter == NULL) return FALSE;

		// 变换 + 加密后异步提交，返回即可复用缓冲区
		BYTE* pOut = TransformAndEncrypt(pPicImg->pDataBuffer);
		char errBuf[512] = {0};
		if (!tszarr_writer_write_packet(m_tsWriter, isn, pOut, errBuf, sizeof(errBuf)))
		{
			qCritical() << "TsZarr write packet" << isn << "failed:" << errBuf;
			g_isSave[m_ichl] = false; // 停止本通道存储，避免静默丢数据
			emit thread_signal_stopsave(m_ichl);
			return FALSE;
		}
	}
	else
	{
		if (lpFile_camData == NULL) return FALSE;

		// 先存camdata,pPicImg.pDataBuffer
		// 先加密再存储
		if (im_encode != NULL)
		{
			im_encode->encrypt_data((char*)pPicImg->pDataBuffer, pPicImg->iBufferSize);
		}
		int a = fwrite(pPicImg->pDataBuffer, pPicImg->iBufferSize, 1, lpFile_camData);
	}

	// 再存camidx，pPicImg.picInfor
	// 这里写索引信息（两种模式照写，zarr 模式下 startPosition 语义为“包号*包大小”）
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

	// 先打开相机数据索引文件（zarr attributes 需要索引文件名）
	OpenCamIdxSaveFile(str_path);

	// 打开相机数据存储：zarr 模式建 zarr 目录，否则打开 .camdatx 文件
	if (m_zarrMode)
	{
		OpenZarrSaveStore(str_path);
	}
	else
	{
		OpenCamDataSaveFile(str_path);
	}

	m_iPicCnt = 0; // 每次存储前，序号归零
	m_pPicture->picInfor.startPosition = 0;


}
void QSaveThd::CreateEncryptorFromFileName(const QString &strAllFile)
{
	// 取出文件名，利用时间和相机名称进行加密，例如"20250514144747-1Cam_ZQJ1"
	QFileInfo fi(strAllFile);
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
}
void QSaveThd::OpenCamDataSaveFile(QString str_path_file)
{
	// 打开文件
	m_strAllfile_camData = str_path_file;

	CreateEncryptorFromFileName(m_strAllfile_camData);

	char xmlPath[RouteInfoSize];
	strcpy(xmlPath, m_strAllfile_camData.toLocal8Bit().data());

	if (fopen_s(&lpFile_camData, xmlPath, "wb") != 0)
	{
		return;
	}

}
void QSaveThd::OpenZarrSaveStore(QString str_path_file)
{
	// 密钥推导与 camdatx 完全一致（同一文件名主干，前两个'-'分段）
	CreateEncryptorFromFileName(str_path_file);

	// 目录名：把 ".camdatx.tmp" 换成 ".zarr.tmp"，收口后去掉 ".tmp"
	QString str1 = QStringLiteral(".camdatx.tmp");
	QString strBase = str_path_file.mid(0, str_path_file.length() - str1.length());
	m_strAllfile_zarrFinal = strBase + QStringLiteral(".zarr");
	m_strAllfile_zarrTmp = m_strAllfile_zarrFinal + QStringLiteral(".tmp");

	// 变换开启时才分配变换输出缓冲
	if (m_camInfor.SaveTransform != 0 && m_pTransformBuf == NULL)
	{
		m_pTransformBuf = new BYTE[m_iPacketBytes];
	}

	// 静态元数据进 zarr attributes（camidx 里的逐帧序列仍写 camidx，不进 zarr）
	QFileInfo fiIdx(strBase + QStringLiteral(".camidx")); // 索引文件收口后的最终名
	QString strAttr = QString("{\"camera_name\":\"%1\",\"encrypted\":true,"
							  "\"frames_per_packet\":%2,\"lines_per_frame\":%3,"
							  "\"image_width\":%4,\"color_channels\":%5,"
							  "\"save_transform\":%6,\"index_file\":\"%7\"}")
						  .arg(m_camInfor.Name)
						  .arg(MAXFRAME)
						  .arg(m_camInfor.ImageHeight)
						  .arg(m_camInfor.ImageWidth)
						  .arg((m_camInfor.isColorCamera != 0) ? 3 : 1)
						  .arg(m_camInfor.SaveTransform)
						  .arg(fiIdx.fileName());
	QByteArray attrUtf8 = strAttr.toUtf8();
	QByteArray pathUtf8 = m_strAllfile_zarrTmp.toUtf8();

	TsZarrWriterOptions opts = {};
	opts.packet_height = m_zarrRows;
	opts.packet_width = m_zarrCols;
	opts.initial_capacity = 256;
	opts.max_pending = 8;
	opts.attributes_json_utf8 = attrUtf8.constData();

	char errBuf[512] = {0};
	m_tsWriter = tszarr_writer_create(pathUtf8.constData(), &opts, errBuf, sizeof(errBuf));
	if (m_tsWriter == NULL)
	{
		qCritical() << "TsZarr create failed for" << m_strAllfile_zarrTmp << ":" << errBuf;
	}
}
void QSaveThd::CloseZarrStore()
{
	if (m_tsWriter == NULL)
	{
		return;
	}
	char errBuf[512] = {0};
	// 等全部落盘并把 packet 维收缩为实际包数
	if (!tszarr_writer_close(m_tsWriter, m_iPicCnt, errBuf, sizeof(errBuf)))
	{
		qCritical() << "TsZarr close failed for" << m_strAllfile_zarrTmp << ":" << errBuf;
	}
	m_tsWriter = NULL;

	// 目录去 .tmp 后缀
	QDir dirRename;
	if (!dirRename.rename(m_strAllfile_zarrTmp, m_strAllfile_zarrFinal))
	{
		qCritical() << "TsZarr rename failed:" << m_strAllfile_zarrTmp;
	}

	QFileInfo fi(m_strAllfile_zarrFinal); // 取出目录名，存台账
	m_str_ImgFileName = fi.fileName();
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

	// zarr 模式：收口（等异步写全部落盘 + 收缩 + 目录去.tmp）
	if (m_zarrMode)
	{
		CloseZarrStore();
	}

	// 关闭文件（zarr 模式下 camData 文件本就未打开，仅关 camidx 和加密器）
	CloseSaveFile();

	// 重命名文件
	if (!m_zarrMode)
	{
		ReNameDatFile();//tFile();
	}
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
