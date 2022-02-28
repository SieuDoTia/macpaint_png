/* MacPaint.h */
#pragma once

/* Dữ Liệu Định Dạng Mac Paint */
typedef struct {
   unsigned char *dau;
   unsigned char *duLieuAnh;
} DuLieuMacPaint;

/* Đọc Tập Tin MacPaint */
DuLieuMacPaint docTapTinMacPaint( FILE *tapTinMacPaint );

/* Lưư Tập Tin MacPaint */
void luuTapTinMacPaint( FILE *tapTinMacPaint, DuLieuMacPaint *duLieuTapTin );