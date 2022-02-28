/* PNG.h */

#pragma once

// ---- thứ dữ liệu màu trong tập tin
#define kPNG_XAM        0x00  // màu xám <------- hỗ trợ loại này
//#define kPNG_BANG_MAU_XAM  0x01  // có bảng màu xám
#define kPNG_BGR            0x02  // màu đỏ, lục, xanh <------- hỗ trợ loại này
//#define kPNG_BANG_MAU       0x03  // có bảng màu đỏ, lục, xanh
#define kPNG_XAM_DUC        0x04  // màu xám, đục  <------- hỗ trợ loại này
//#define kPNG_BANG_MAU_XAM_DUC 0x05  // có bảng màu xám đục
#define kPNG_BGRO            0x06  // màu đỏ, lục, xanh, đục <------- hỗ trợ loại này
//#define kPNG_BANG_MAU_DUC     0x07  // có bảng màu đỏ, lục, xanh, đục
#define kPNG_CHUA_BIET     0xff  // loại PNG chưa biết


/* đọc tệp PNG BGRO */
unsigned char *docPNG( char *duongTapTin, unsigned int *beRong, unsigned int *beCao, unsigned char *canLatMau, unsigned char *loai );

/* Lưu ảnh PNG */
void luuAnhPNG( char *tenTep, unsigned char *suKhacBiet, unsigned int beRong, unsigned int beCao, unsigned char loai );

