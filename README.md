# Sisop-2-2025-IT35
### Anggota Kelompok:  
- Rayka Dharma Pranandita (*5027241039*)
- Bima Aria Perthama (*5027241060*)
- Gemilang Ananda Lingua (*5027241072*)

### Table of Contents :
- [Nomor 1](#nomor-1-gilang)
- [Nomor 2](#nomor-2-rayka)  
  a. Relics File Reading  
  b. Mount_dir file manipulation  
  c. New file in mount_dir chunked in relics
  d. Mount_dir file complete deletion
  e. FUSE activity log
- [Nomor 3](#nomor-3-rayka)
- [Nomor 4](#nomor-4-aria)

### Nomor 1 (Gilang)   
### Nomor 2 (Rayka)   
### Relics File Reading
### Mount_dir file manipulation
### New file in mount_dir chunked in relics
### Mount_dir file complete deletion
### FUSE activity log
Dalam melakukan loging, digunakan fungsi utama berupa *write_log()* yang tertulis sebagai berikut:   
```c
void write_log(const char *format, ...) {
    FILE *log = fopen(LOG_FILE, "a");
    if (!log) return;

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    fprintf(log, "[%04d-%02d-%02d %02d:%02d:%02d] ",
            t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
            t->tm_hour, t->tm_min, t->tm_sec);

    va_list args;
    va_start(args, format);
    vfprintf(log, format, args);
    va_end(args);

    fprintf(log, "\n");
    fclose(log);
}
```
Jadi fungsi akan menerima input yang berupa format dan ellipsis (...), dimana ellipsis merupakan paramater yang tidak diklasifikasikan dan dapat diisi oleh lebih dari satu args atau avriable, hal ini akan lebih jelas ketika masuk ke tiap pemanggilan fungsi. Jadi dalam fungsi dilakukan pembuatan file log secara append, dan menggambil waktu sekarang dengan localtime(&now) dan mengubah format timestamp menjadi [YEAR-MM-DD hh-mm-ss\]. Selanjutnya va_lists menerima list variable args dan va_start melakukan pengurutan argument yang dimulai setelah variable format, vfprintf mirip dengan fprintf, namun melibatkan args tadi, dan va_end mengakhiri akses args dan membersihkan sumber daya. Sehingga pada hasil akhir logs akan berbentuk [YEAR-MM-DD hh-mm-ss\] FORMAT: args.

Pemanggilan fungsi *write_log*:
- Pada saat membuka file (vfs_open):
```c
```
- Pada saat copy file ke mount_dir (vfs_create):
```c
```
- Pada saat memecah file atau *chunking* (vfs_write):
```c
```
- Pada saat menghapus file di mount_dir (vfs_unlink):
```c
```
### Nomor 3 (Rayka)   
### Nomor 4 (Aria)
