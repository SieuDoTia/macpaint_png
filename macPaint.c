// mã nguồn cho đọc và lưu tập tin MacPaint
// 2022.01.20

// +------------------+
// |   đầu 512 byte   |
// +------------------+
// |                  |
// |                  |
// |  576 x 720 bit   |
// |  (72 x 720) byte |
// |                  |
// |                  |
// +------------------+

// Lưu ý: dữ liệu ảnh có lẻ được nén
//     Trường hợp tội tệ nhất là kèm một thêm cho mỗ hành, tức là 720 byte
//     Cho đệm nên bộ nhớ tối thiểu cần chứa tập tin MacPaint là:
//     512 + 72 x 720 + 720 = 53 072 byte

// Hình như ảnh MacPaint chỉ có cỡ thước 576 x 720 điểm ảnh

/* ĐẦU (36 BYTE)
 0  uchar phiên bản = 0
 1  uchar bề dài tên tập tin
 2  uchar tên tập tin 63 byte

65  uint  mã loại tập tin = 'PNTG'
69  uint  mã ứng dụng chế tạo tập tin ('MPNT' cho ứng dụng MacPaint)
73  uchar lã cờ tập tin
74  uchar giữ trư 1
75  short vi trí y
77  short vị trí x
79  ushort số cửa sổ
81  uchar được bảo vệ
82  uchar giữ trư 2
83  uint chiều dài nhánh dữ liệu
87  uint chiều dài nhánh tài nguyên = 0
91  uint dấu mọc chê tạo
95  uint dấu móc chỉnh sửa
97  ushort bề dài chú dẫn
 */

/*
128 thông tin tập tin
132 38 họa tiết x 8 byte = 304 byte
 
436 204 byte 0x00

640 dũ liệu ảnh (720 hàng x 72 byte)
 */


#include <stdio.h>
#include <stdlib.h>
#include "macPaint.h"
#include "PNG.h"


#define kPNG  2
#define kMAC  3


#pragma mark ----- Rã RLE
void uncompress_rle( char *compressed_buffer, int compressed_buffer_length, unsigned char *uncompressed_buffer, int uncompressed_buffer_length) {
   
   // ---- while not finish all in buffer
   while( compressed_buffer_length > 0 ) {
      char byteCount = *compressed_buffer;
      // ---- move to next byte
      compressed_buffer++;
      
      // ---- if byte value ≥ 0
      if( byteCount > -1 ) {
         // ---- count for not same byte value
         int count = byteCount + 1;
         //         printf( "  chép count %d ", count );
         // ---- reduce amount count of bytes remaining for in buffer
         compressed_buffer_length -= count;
         compressed_buffer_length--;
         // ---- if count larger than out buffer length still available
         if (0 > (uncompressed_buffer_length -= count)) {
            printf( "  uncompress_rle: Error buffer not length not enough. Have %d but need %d\n", uncompressed_buffer_length, count );
            exit(0);
            return;
         }
         // ---- copy not same byte and move to next byte
         while( count-- > 0 )
            *uncompressed_buffer++ = *(compressed_buffer++);
         
      }
      else if( byteCount < 0 ) {
         // ---- count number bytes same
         int count = -byteCount+1;
         unsigned char valueForCopy = *compressed_buffer;
         //         printf( "  repeat count %d (%d) ", count, valueForCopy );
         // ---- reduce amount count of remaining bytes for in buffer
         compressed_buffer_length -= 2;
         // ---- if count larger than out buffer length still available
         if (0 > (uncompressed_buffer_length -= count)) {
            printf( "  uncompress_rle: Error buffer not length not enough. Have %d but need %d\n", uncompressed_buffer_length, count );
            exit(0);
            return;
         }
         // ---- copy same byte
         while( count-- > 0 )
            *uncompressed_buffer++ = valueForCopy;
         // ---- move to next byte
         compressed_buffer++;
      }
      
   }
   
}


#pragma mark ---- Nén RLE
// ---- Hàm từ thư viện OpenEXR của ILM
#define MIN_RUN_LENGTH  3
#define MAX_RUN_LENGTH 72 // thường == 127 nhưng tập tin MacPaint chỉ dùng 72 từng hàng 
unsigned int compress_chunk_RLE(unsigned char *in, unsigned int inLength, unsigned char *out ) {
   const unsigned char *inEnd = in + inLength;
   const unsigned char *runStart = in;
   const unsigned char *runEnd = in + 1;
   unsigned char *outWrite = out;
   
   // ---- while not at end of in buffer
   while (runStart < inEnd) {
      
      // ---- count number bytes same value; careful not go beyond end of chunk, or chunk longer than MAX_RUN_LENGTH
      while (runEnd < inEnd && *runStart == *runEnd && runEnd - runStart - 1 < MAX_RUN_LENGTH) {
         ++runEnd;
      }
      // ---- if number bytes same value >= MIN_RUN_LENGTH
      if (runEnd - runStart >= MIN_RUN_LENGTH) {
         //
         // Compressable run
         //
         // ---- number bytes same value - 1
         char count = (runEnd - runStart) - 1;
         *outWrite++ = -count;
         // ---- byte value
         *outWrite++ = *(signed char *) runStart;
         // ---- move to where different value found or MAX_RUN_LENGTH
         runStart = runEnd;
      }
      else {
         //
         // Uncompressable run
         //
         // ---- count number of bytes not same; careful end of chunk,
         while (runEnd < inEnd &&
                ((runEnd + 1 >= inEnd || *runEnd != *(runEnd + 1)) || (runEnd + 2 >= inEnd || *(runEnd + 1) != *(runEnd + 2))) &&
                runEnd - runStart < MAX_RUN_LENGTH) {
            ++runEnd;
         }
         // ---- number bytes not same
         *outWrite++ = runEnd - runStart - 1;
         // ---- not same bytes
         while (runStart < runEnd) {
            *outWrite++ = *(signed char *) (runStart++);
         }
      }
      // ---- move to next byte
      ++runEnd;
   }
   
   return outWrite - out;
}



#pragma mark ---- Đọc Tập Tin MacPaint
DuLieuMacPaint docTapTinMacPaint( FILE *tapTinMacPaint ) {

   DuLieuMacPaint duLieuMacPaint;
   
   unsigned int uncompressed_data_length = 72*720;  // 576/8 = 72 byte
   
   char *demNen = malloc( 73*720 );

   if( demNen == NULL ) {
      printf( "Problem create compressed buffer\n" );
      exit(0);
   }
   

   // ==== đầu
   fseek(tapTinMacPaint, 1, SEEK_SET );
   unsigned char beDaiTen = fgetc( tapTinMacPaint );
   char tenTapTin[64];
   unsigned short chiSo = 0;
   while( chiSo < beDaiTen ) {
      tenTapTin[chiSo] = fgetc( tapTinMacPaint );
      chiSo++;
   }
   tenTapTin[chiSo] = 0; // kết thúc tên
   printf( " Tên tập tin: %s\n", tenTapTin );
   
   // ---- mã tập tin
   fseek( tapTinMacPaint, 65, SEEK_SET );
   unsigned int maTapTin = fgetc( tapTinMacPaint ) << 24 | fgetc( tapTinMacPaint ) << 16 | fgetc( tapTinMacPaint ) << 8 | fgetc( tapTinMacPaint );
   unsigned int maUngDungTao = fgetc( tapTinMacPaint ) << 24 | fgetc( tapTinMacPaint ) << 16 | fgetc( tapTinMacPaint ) << 8 | fgetc( tapTinMacPaint );

   // ---- vị trí
   fseek( tapTinMacPaint, 75, SEEK_SET );
   short viTriY = fgetc( tapTinMacPaint ) << 8 | fgetc( tapTinMacPaint );
   short viTriX = fgetc( tapTinMacPaint ) << 8 | fgetc( tapTinMacPaint );
   printf( " vị trí: (%d; %d)\n", viTriX, viTriY );
   
   // ---- dữ liệu ảnh
   fseek( tapTinMacPaint, 640, SEEK_SET );
   duLieuMacPaint.duLieuAnh = malloc( 720*72*2 );

   if( duLieuMacPaint.duLieuAnh == NULL ) {
      printf( " Vấn đề tạo dệm chứa dữ liệu tập tin %s\n", tenTapTin );
      exit(0);
   }

   // ---- đọc ảnh
   unsigned int beDaiDemNen = 0;
   while( !feof(tapTinMacPaint) ) {
      demNen[beDaiDemNen] = fgetc( tapTinMacPaint );
      beDaiDemNen++;
   }
   printf( "  beDaiDemNen %d\n", beDaiDemNen );
   
   // ---- rã ảnh
   unsigned char *duLieu = duLieuMacPaint.duLieuAnh;
   uncompress_rle( demNen, beDaiDemNen, duLieu, 72*720*2 );

   return duLieuMacPaint;
}

#pragma mark ---- Lưu Tập Tin MacPaint
// Lưư ý: chỉ lưu 72 byte cho mỗi hàng (576 bit)
void luuTapTinMacPaint( FILE *tapTinMacPaint, DuLieuMacPaint *duLieuTapTin ) {

   unsigned int uncompressed_data_length = 72;  // 576/ 8 = 72 byte
   
   char *compressed_buffer = malloc( 73 );
   unsigned char *uncompressed_buffer = malloc( uncompressed_data_length );
   if( compressed_buffer == NULL ) {
      printf( "Problem create compressed buffer\n" );
      exit(0);
   }
   
   if( uncompressed_buffer == NULL ) {
      printf( "Problem create uncompressed buffer\n" );
      exit(0);
   }

   // ---- lưu đầu
   unsigned int chiSo = 0;
   unsigned char *dau = duLieuTapTin->dau;
   while( chiSo < 512 ) {
      fputc( *dau, tapTinMacPaint );
      dau++;
      chiSo++;
   }
   
   // ---- nén dữ liệu ảnh
   unsigned char *duLieu = duLieuTapTin->duLieuAnh;
   unsigned int soHang = 0;
   
   while( soHang < 720 ) {
      
      // ---- nén

      soHang++;
   }
}

#pragma mark ---- Phân Tích Đuôi Tập Tin
unsigned char phanTichDuoiTapTin( char *tenTapTin ) {
   
   // ---- đến cuối cùnh tên
   while( *tenTapTin != 0x00 ) {
      tenTapTin++;
   }
   
   // ---- trở lại 3 cái
   tenTapTin -= 3;
   
   // ---- xem có đuôi nào
   unsigned char kyTu0 = *tenTapTin;
   tenTapTin++;
   unsigned char kyTu1 = *tenTapTin;
   tenTapTin++;
   unsigned char kyTu2 = *tenTapTin;
   
   unsigned char loaiTapTin = 0;
   
   if( (kyTu0 == 'p') || (kyTu0 == 'P')  ) {
      if( (kyTu1 == 'n') || (kyTu1 == 'N')  ) {
         if( (kyTu2 == 'g') || (kyTu2 == 'G')  ) {
            loaiTapTin = kPNG;
         }
      }
   }
   else if( (kyTu0 == 'm') || (kyTu0 == 'M')  ) {
      if( (kyTu1 == 'a') || (kyTu1 == 'A')  ) {
         if( (kyTu2 == 'c') || (kyTu2 == 'C')  ) {
            loaiTapTin = kMAC;
         }
      }
   }
   
   
   return loaiTapTin;
}

#pragma mark ==== Đuôi Tập Tin
void tenAnhPNG( char *tenAnhGoc, char *tenAnhPNG ) {
   
   // ---- chép tên ảnh gốc
   while( *tenAnhGoc != 0x00 ) {
      *tenAnhPNG = *tenAnhGoc;
      tenAnhPNG++;
      tenAnhGoc++;
   }
   
   // ---- trở lại 3 cái
   tenAnhPNG -= 3;
   
   // ---- kèm đuôi PNG
   *tenAnhPNG = 'p';
   tenAnhPNG++;
   *tenAnhPNG = 'n';
   tenAnhPNG++;
   *tenAnhPNG = 'g';
   tenAnhPNG++;
   *tenAnhPNG = 0x0;
}

void tenAnhMacPaint( char *tenAnhGoc, char *tenAnhMac ) {
   
   // ---- chép tên ảnh gốc
   while( *tenAnhGoc != 0x00 ) {
      *tenAnhMac = *tenAnhGoc;
      tenAnhMac++;
      tenAnhGoc++;
   }
   
   // ---- trở lại 3 cái
   tenAnhMac -= 3;
   
   // ---- kèm đuôi PNG
   *tenAnhMac = 'p';
   tenAnhMac++;
   *tenAnhMac = 'c';
   tenAnhMac++;
   *tenAnhMac = 't';
   tenAnhMac++;
   *tenAnhMac = 0x0;
}


#pragma mark ==== main.c
int main( int argc, char **argv ) {
   
   char tenTapTin[256] = "MacPaint.mpt";
   
   if( argc > 1 ) {
      // ---- phân tích đuôi tập tin
      unsigned char loaiTapTin = 0;
      loaiTapTin = phanTichDuoiTapTin( argv[1] );
      printf( "loaiTapTin %d\n", loaiTapTin );
      
      if( loaiTapTin == kMAC ) {

         // ---- mở tập tin
         FILE *tapTinMacPaint = fopen( argv[1], "r" );
         
         if( tapTinMacPaint != NULL ) {
            // ---- đoc ảnh
            DuLieuMacPaint anh = docTapTinMacPaint( tapTinMacPaint );
            fclose( tapTinMacPaint );
            
            // ---- đổi sang PNG
            // ---- chuẩn bị tên tập tin
            char tenTep[255];
            tenAnhPNG( argv[1], tenTep );
            
            // ---- pha trộn các kênh
            unsigned int chiSoCuoi = 720*576 << 2;
            unsigned char *demPhaTron = malloc( chiSoCuoi );
            
            if( demPhaTron != NULL ) {
               
               unsigned int chiSo = 0;
               short hang = 719;
               unsigned int chiSoByte = hang*72;
               unsigned char soByteTrongHang = 0;
               
               while( chiSo < chiSoCuoi ) {
                  
                  unsigned char byte = anh.duLieuAnh[chiSoByte];
                  unsigned char giaTri = 0;
                  
                  // ---- điểm ảnh 0
                  if( byte & 0x80 )
                     giaTri = 0;
                  else
                     giaTri = 255;
                  
                  demPhaTron[chiSo] = giaTri;
                  demPhaTron[chiSo+1] = giaTri;
                  demPhaTron[chiSo+2] = giaTri;
                  demPhaTron[chiSo+3] = 0xff;
                  chiSo += 4;
                  
                  // ---- điểm ảnh 1
                  if( byte & 0x40 )
                     giaTri = 0;
                  else
                     giaTri = 255;
                  
                  demPhaTron[chiSo] = giaTri;
                  demPhaTron[chiSo+1] = giaTri;
                  demPhaTron[chiSo+2] = giaTri;
                  demPhaTron[chiSo+3] = 0xff;
                  chiSo += 4;
                  
                  // ---- điểm ảnh 2
                  if( byte & 0x20 )
                     giaTri = 0;
                  else
                     giaTri = 255;
                  
                  demPhaTron[chiSo] = giaTri;
                  demPhaTron[chiSo+1] = giaTri;
                  demPhaTron[chiSo+2] = giaTri;
                  demPhaTron[chiSo+3] = 0xff;
                  chiSo += 4;
                  
                  // ---- điểm ảnh 3
                  if( byte & 0x10 )
                     giaTri = 0;
                  else
                     giaTri = 255;
                  
                  demPhaTron[chiSo] = giaTri;
                  demPhaTron[chiSo+1] = giaTri;
                  demPhaTron[chiSo+2] = giaTri;
                  demPhaTron[chiSo+3] = 0xff;
                  chiSo += 4;
                  
                  // ---- điểm ảnh 4
                  if( byte & 0x08 )
                     giaTri = 0;
                  else
                     giaTri = 255;
                  
                  demPhaTron[chiSo] = giaTri;
                  demPhaTron[chiSo+1] = giaTri;
                  demPhaTron[chiSo+2] = giaTri;
                  demPhaTron[chiSo+3] = 0xff;
                  chiSo += 4;
                  
                  // ---- điểm ảnh 5
                  if( byte & 0x04 )
                     giaTri = 0;
                  else
                     giaTri = 255;
                  
                  demPhaTron[chiSo] = giaTri;
                  demPhaTron[chiSo+1] = giaTri;
                  demPhaTron[chiSo+2] = giaTri;
                  demPhaTron[chiSo+3] = 0xff;
                  chiSo += 4;
                  
                  // ---- điểm ảnh 6
                  if( byte & 0x02 )
                     giaTri = 0;
                  else
                     giaTri = 255;
                  
                  demPhaTron[chiSo] = giaTri;
                  demPhaTron[chiSo+1] = giaTri;
                  demPhaTron[chiSo+2] = giaTri;
                  demPhaTron[chiSo+3] = 0xff;
                  chiSo += 4;
                  
                  // ---- điểm ảnh 7
                  if( byte & 0x01 )
                     giaTri = 0;
                  else
                     giaTri = 255;
                  
                  demPhaTron[chiSo] = giaTri;
                  demPhaTron[chiSo+1] = giaTri;
                  demPhaTron[chiSo+2] = giaTri;
                  demPhaTron[chiSo+3] = 0xff;
                  chiSo += 4;
                  
                  chiSoByte++;
                  soByteTrongHang++;
                  if( soByteTrongHang == 72 ) {
                     soByteTrongHang = 0;
                     hang--;
                     chiSoByte = hang*72;
                  }
               }
               
               // ---- lưu tập tin PNG
               luuAnhPNG( tenTep, demPhaTron, 72 << 3, 720, kPNG_BGRO );
            }
            
            // -----
            free( anh.duLieuAnh );
         }
      }
      else if( loaiTapTin == kPNG ) {
         
      }
   }
   return 0;
}