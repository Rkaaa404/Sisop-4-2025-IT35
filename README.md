# Sisop-2-2025-IT35
### Anggota Kelompok:  
- Rayka Dharma Pranandita (*5027241039*)
- Bima Aria Perthama (*5027241060*)
- Gemilang Ananda Lingua (*5027241072*)

### Table of Contents :
- [Nomor 1](#nomor-1-gilang)   
  a. Downloading and Preparing   
  b. Hexadecimal to Image Conversion   
  c. Image File Naming Convention   
  d. Logging Conversion Activities   
- [Nomor 2](#nomor-2-rayka)  
  a. Directory Management  
  b. Mount_dir file manipulation  
  c. New file in mount_dir chunked in relics    
  d. Mount_dir file complete deletion     
  e. FUSE activity log
- [Nomor 3](#nomor-3-rayka)  
  a. Docker Init  
  b. Filename detection and reversing  
  c. Normal txt file text ROT13    
  d. Logger     
  e. File changes only in container
- [Nomor 4](#nomor-4-aria)

### Nomor 1 (Gilang) 
### Downloading and Preparing 
Diberikan sebuah link download file dari Google Drive yang berupa beberapa file ber extensi .txt. Isi file ini merupakan file-file yang sudah dirubah menjadi hexadeximal. Target pada soal ini adalah, kita harus bisa download, meng-unzipnya, lalu otomatis menghapus file zip nya. Berikut code untuk memenuhi target-target dari sub soal A
```c
#define DOWNLOAD_URL "https://drive.google.com/uc?id=1hi_GDdP51Kn2JJMw02WmCOxuc3qrXzh5&export=download"
#define ZIP_NAME "data.zip"
#define SOURCE_FOLDER "anomali"

int file_exists(const char *path) {
    struct stat st;
    return stat(path, &st) == 0;
}

void download_and_extract() {
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
        "wget -q --show-progress -O %s \"%s\" && unzip -o %s && rm %s",
        ZIP_NAME, DOWNLOAD_URL, ZIP_NAME, ZIP_NAME);
    system(cmd);
    printf("Data berhasil diunduh dan diekstrak.\n");
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <mountpoint>\n", argv[0]);
        return 1;
    }

    if (!file_exists(SOURCE_FOLDER)) {
        printf("Folder %s belum ada, mulai download...\n", SOURCE_FOLDER);
        download_and_extract();
    }

    process_all_files();

    char *fuse_argv[] = { argv[0], argv[1], "-o", "nonempty" };
    return fuse_main(4, fuse_argv, &operations, NULL);
}
```
### Hexadecimal to Image Conversion
Selanjutnya, yang dibutuhkan pada soal merupakan code yang mengonversi isi hexadecimal menjadi biner dan menyimpannya sebagai gambar PNG ke dalam folder image/
```c
#define IMAGE_FOLDER "image"

static int is_hex_char(char c) {
    return (c >= '0' && c <= '9') || 
           (c >= 'a' && c <= 'f') || 
           (c >= 'A' && c <= 'F');
}

void create_image_directory() {
    struct stat st = {0};
    if (stat(IMAGE_FOLDER, &st) == -1) {
        mkdir(IMAGE_FOLDER, 0755);
    }
}

int hex_to_png(const char *filepath) {
    FILE *fp = fopen(filepath, "r");
    if (!fp) return 0;

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    rewind(fp);

    char *hex_data = malloc(size + 1);
    fread(hex_data, 1, size, fp);
    hex_data[size] = '\0';
    fclose(fp);

    size_t len = strlen(hex_data);
    char *bin = malloc(len / 2);
    size_t bin_idx = 0;
    for (size_t i = 0; i < len; i += 2) {
        if (!is_hex_char(hex_data[i]) || !is_hex_char(hex_data[i + 1])) continue;
        char byte_str[3] = {hex_data[i], hex_data[i + 1], '\0'};
        bin[bin_idx++] = (char)strtol(byte_str, NULL, 16);
    }

    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    char imgname[100];
    const char *base = strrchr(filepath, '/');
    snprintf(imgname, sizeof(imgname), IMAGE_FOLDER"/%s_image_%04d-%02d-%02d_%02d:%02d:%02d.png",
             base ? base + 1 : filepath,
             tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
             tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);

    FILE *out = fopen(imgname, "wb");
    if (!out) {
        free(hex_data);
        free(bin);
        return 0;
    }
    fwrite(bin, 1, bin_idx, out);
    fclose(out);

    free(hex_data);
    free(bin);
    return 1;
}

void process_all_files() {
    create_image_directory();
    DIR *dir = opendir(SOURCE_FOLDER);
    if (!dir) return;

    struct dirent *ent;
    char path[1024];
    while ((ent = readdir(dir)) != NULL) {
        if (ent->d_type == DT_REG && strstr(ent->d_name, ".txt")) {
            snprintf(path, sizeof(path), SOURCE_FOLDER"/%s", ent->d_name);
            int success = hex_to_png(path);
            log_conversion(ent->d_name, success);
        }
    }
    closedir(dir);
}
```
### Image File Naming Convention
Selanjutnya, soal mengharapkan file gambar disimpan dengan format : 
```bash
[nama file]_image_[YYYY-mm-dd]_[HH:MM:SS].png
```
yang selanjutnya kita menggunakan code 
```c
snprintf(imgname, sizeof(imgname), IMAGE_FOLDER"/%s_image_%04d-%02d-%02d_%02d:%02d:%02d.png",
         base ? base + 1 : filepath,
         tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
         tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
```
untuk menyimpan filenya dengan format yang sudah diharapkan.
### Logging Conversion Activities
Sub soal terakhir, mengharapkan setiap hasil konversi (baik sukses maupun gagal) dicatat ke file conversion.log dengan format waktu dan status.
```c
#define LOG_FILE "conversion.log"

void log_conversion(const char *filename, int success) {
    FILE *log = fopen(LOG_FILE, "a");
    if (log) {
        time_t t = time(NULL);
        struct tm *tm_info = localtime(&t);
        char time_str[30];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
        fprintf(log, "[%s] %s: %s\n", time_str, filename, success ? "SUCCESS" : "FAILED");
        fclose(log);
    }
}
```
### FUSE Operation
```c
static int x_getattr(const char *path, struct stat *st, struct fuse_file_info *fi) {
    char fullpath[1024];
    snprintf(fullpath, sizeof(fullpath), "%s%s", SOURCE_FOLDER, path);
    return lstat(fullpath, st);
}

static int x_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                     off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags) {
    DIR *dp;
    struct dirent *de;
    char fullpath[1024];
    snprintf(fullpath, sizeof(fullpath), "%s%s", SOURCE_FOLDER, path);

    dp = opendir(fullpath);
    if (!dp) return -errno;

    while ((de = readdir(dp)) != NULL) {
        filler(buf, de->d_name, NULL, 0, 0);
    }
    closedir(dp);
    return 0;
}

static int x_open(const char *path, struct fuse_file_info *fi) {
    char fullpath[1024];
    snprintf(fullpath, sizeof(fullpath), "%s%s", SOURCE_FOLDER, path);
    int res = open(fullpath, fi->flags);
    if (res == -1) return -errno;
    close(res);
    return 0;
}

static int x_read(const char *path, char *buf, size_t size, off_t offset,
                  struct fuse_file_info *fi) {
    char fullpath[1024];
    snprintf(fullpath, sizeof(fullpath), "%s%s", SOURCE_FOLDER, path);
    FILE *f = fopen(fullpath, "r");
    if (!f) return -errno;
    fseek(f, offset, SEEK_SET);
    int res = fread(buf, 1, size, f);
    fclose(f);
    return res;
}

static const struct fuse_operations operations = {
    .getattr = x_getattr,
    .readdir = x_readdir,
    .open = x_open,
    .read = x_read,
};
```

### Nomor 2 (Rayka)   
### Initial Download
Dalam soal, tidak dijelaskan bahwa download harus dilakukan oleh file baymax.c, sehingga dalam kasus ini saya membuat script bash sendiri untuk melakukan download relics baymas, yaitu sebagai berikut:
```bash
#!/bin/bash
if  [[ ! -f relics/Baymax.jpeg.013 ]]
then
    mkdir -m 0777 relics
    wget -q -O Baymax.zip "https://drive.usercontent.google.com/u/0/uc?id=1MHVhFT57Wa9Zcx69Bv9j5ImHc9rXMH1c&export=download"
    unzip Baymax.zip -d relics
    rm Baymax.zip
    echo "Relics Baymax berhasil didapatkan"
else 
    echo "Relics Baymax sudah ada"
fi
```
### Directory Management
```c
static int vfs_getattr(const char *path, struct stat *stbuf) {
    memset(stbuf, 0, sizeof(struct stat));

    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    } else {
        char realpath[256];
        snprintf(realpath, sizeof(realpath), RELICS_DIR"%s.000", path + 1);
        if (access(realpath, F_OK) == 0) {
            stbuf->st_mode = S_IFREG | 0644;

            off_t total = 0;
            int i = 0;
            while (1) {
                snprintf(realpath, sizeof(realpath), RELICS_DIR"%s.%03d", path + 1, i++);
                FILE *f = fopen(realpath, "rb");
                if (!f) break;
                fseek(f, 0, SEEK_END);
                total += ftell(f);
                fclose(f);
            }
            stbuf->st_size = total;
            stbuf->st_nlink = 1;
        } else {
            return -ENOENT;
        }
    }
    return 0;
}
```
Mendapatkan attribute dari file dan dir mulai dari size dan seterusnya
```c
static int vfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    (void) offset; (void) fi;

    if (strcmp(path, "/") != 0)
        return -ENOENT;

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    DIR *dp = opendir(RELICS_DIR);
    if (!dp) return -errno;

    struct dirent *de;
    char lastfile[256] = "";

    while ((de = readdir(dp)) != NULL) {
        char *dot = strrchr(de->d_name, '.');
        if (!dot || strlen(dot) != 4) continue;

        char filename[256];
        strncpy(filename, de->d_name, dot - de->d_name);
        filename[dot - de->d_name] = '\0';

        if (strcmp(lastfile, filename) != 0) {
            filler(buf, filename, NULL, 0);
            strcpy(lastfile, filename);
        }
    }

    closedir(dp);
    return 0;
}
```
Membaca directory dan mendapatkan nama nama file yang ada di dalamnya
### Mount_dir file manipulation
```c
static int vfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    (void) fi;
    size_t total_read = 0;
    int chunk_index = offset / CHUNK_SIZE;
    size_t chunk_offset = offset % CHUNK_SIZE;

    while (size > 0) {
        char chunkpath[256];
        snprintf(chunkpath, sizeof(chunkpath), RELICS_DIR"%s.%03d", path + 1, chunk_index);

        FILE *f = fopen(chunkpath, "rb");
        if (!f) break;

        fseek(f, chunk_offset, SEEK_SET);
        size_t bytes = fread(buf + total_read, 1, CHUNK_SIZE - chunk_offset, f);
        fclose(f);

        if (bytes == 0) break;

        total_read += bytes;
        size -= bytes;
        chunk_offset = 0;
        chunk_index++;
    }

    return total_read;
}
```
Membaca file dengan melakukan read binary chunk file yang ada di relics directory
Hasil Baymax:
![Baymax Read](https://github.com/Rkaaa404/Sisop-4-2025-IT35/blob/main/assets/baymaxRead.png)
Hasil Image yang dicopy:
![Copied Image Read](https://github.com/Rkaaa404/Sisop-4-2025-IT35/blob/main/assets/copyResult.png)
### New file in mount_dir chunked in relics
```c
static int vfs_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    (void) fi;

    char chunkpath[256];
    snprintf(chunkpath, sizeof(chunkpath), RELICS_DIR"%s.000", path + 1);

    write_log("COPY: %s", path + 1);

    FILE *f = fopen(chunkpath, "wb");
    if (!f) return -EACCES;

    fclose(f);
    return 0;
}
```
Jika ada file baru, yang dicopy ke mount_dir. Maka akan membuat log copy, dan membuat file chunk pertama (index 000)
```c
static int vfs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    (void) fi;
    size_t bytes_written = 0;
    char chunk_list[1024] = "";  // untuk menyimpan daftar chunk (untuk log)

    while (bytes_written < size) {
        int chunk_index = (offset + bytes_written) / CHUNK_SIZE;
        int chunk_offset = (offset + bytes_written) % CHUNK_SIZE;
        size_t write_size = CHUNK_SIZE - chunk_offset;
        if (write_size > size - bytes_written) {
            write_size = size - bytes_written;
        }

        char chunkpath[256];
        snprintf(chunkpath, sizeof(chunkpath), RELICS_DIR"%s.%03d", path + 1, chunk_index);

        FILE *f = fopen(chunkpath, "r+b");
        if (!f) f = fopen(chunkpath, "wb");
        if (!f) return -EACCES;

        fseek(f, chunk_offset, SEEK_SET);
        fwrite(buf + bytes_written, 1, write_size, f);
        fclose(f);

        // Tambahkan ke daftar chunk untuk log
        char chunkname[64];
        snprintf(chunkname, sizeof(chunkname), "%s.%03d", path + 1, chunk_index);
        if (!strstr(chunk_list, chunkname)) {  // hindari duplikasi jika write ke offset yg sama
            if (strlen(chunk_list) > 0) strcat(chunk_list, ", ");
            strcat(chunk_list, chunkname);
        }

        bytes_written += write_size;
    }

    write_log("WRITE: %s -> %s", path + 1, chunk_list);  // log hasil pemecahan
    return size;
}
```
Membagi file menjadi potongan 1KB hingga mencapai size dari file yang ada di mount_dir    
![Tree Copy](https://github.com/Rkaaa404/Sisop-4-2025-IT35/blob/main/assets/copyTree.png)
### Mount_dir file complete deletion
```c
static int vfs_unlink(const char *path) {
    int deleted = 0;
    char chunkpath[256];
    char delrange[256] = "";

    for (int i = 0;; i++) {
        snprintf(chunkpath, sizeof(chunkpath), RELICS_DIR"%s.%03d", path + 1, i);
        if (access(chunkpath, F_OK) != 0) break;

        if (deleted == 0)
            snprintf(delrange, sizeof(delrange), "%s.%03d", path + 1, i);
        deleted++;

        remove(chunkpath);
    }

    if (deleted > 0)
        write_log("DELETE: %s - %s.%03d", delrange, path + 1, deleted - 1);

    return 0;
}
```
Mengambil file dengan nama serupa dan melakukan penghapusan di relics directory, dilakukan iterasi hingga index file tersebut tidak ada. Gambaran hasil prosedur:   
![rm tree](https://github.com/Rkaaa404/Sisop-4-2025-IT35/blob/main/assets/remTree.png)
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
Jadi fungsi akan menerima input yang berupa format dan ellipsis (...), dimana ellipsis merupakan paramater yang tidak diklasifikasikan dan dapat diisi oleh lebih dari satu variable args, hal ini akan lebih jelas ketika masuk ke tiap pemanggilan fungsi. Jadi dalam fungsi dilakukan pembuatan file log secara append, dan menggambil waktu sekarang dengan localtime(&now) dan mengubah format timestamp menjadi [YEAR-MM-DD hh-mm-ss\]. Selanjutnya va_lists menerima list variable args dan va_start melakukan pengurutan argument yang dimulai setelah variable format, vfprintf mirip dengan fprintf, namun melibatkan args tadi, dan va_end mengakhiri akses args dan membersihkan sumber daya. Sehingga pada hasil akhir logs akan berbentuk [YEAR-MM-DD hh-mm-ss\] FORMAT: args.

Pemanggilan fungsi *write_log*:
- Pada saat membuka file (vfs_open):
```c
write_log("READ: %s", path + 1);
```
Ketika aktivitas membuka file atau READ, maka akan melakukan logging otomatis aktivitas read dengan contoh output:
![Log Read](https://github.com/Rkaaa404/Sisop-4-2025-IT35/blob/main/assets/readLog.png)
- Pada saat copy file ke mount_dir (vfs_create):
```c
write_log("COPY: %s", path + 1);
```
Hasil log:   
![Log Copy](https://github.com/Rkaaa404/Sisop-4-2025-IT35/blob/main/assets/copyLog.png)
- Pada saat memecah file atau *chunking* (vfs_write):
```c
    while (bytes_written < size) {
        ....
        char chunkname[64];
        snprintf(chunkname, sizeof(chunkname), "%s.%03d", path + 1, chunk_index);
        if (!strstr(chunk_list, chunkname)) {  // hindari duplikasi jika write ke offset yg sama
            if (strlen(chunk_list) > 0) strcat(chunk_list, ", ");
            strcat(chunk_list, chunkname);
        }

        bytes_written += write_size;
    }

    write_log("WRITE: %s -> %s", path + 1, chunk_list);
```
Jadi saat proses pemecahan, dilakukan pencatatan nama file yang dibuat ke dalam chunk_list, selanjutnya akan digunakan dalam *write_log()* sehingga akan menghasilkan output seperti ini:
![Log Write](https://github.com/Rkaaa404/Sisop-4-2025-IT35/blob/main/assets/writeLog.png)
- Pada saat menghapus file di mount_dir (vfs_unlink):
```c
    for (int i = 0;; i++) {
        snprintf(chunkpath, sizeof(chunkpath), RELICS_DIR"%s.%03d", path + 1, i);
        if (access(chunkpath, F_OK) != 0) break;

        if (deleted == 0)
            snprintf(delrange, sizeof(delrange), "%s.%03d", path + 1, i);
        deleted++;

        remove(chunkpath);
    }

    if (deleted > 0)
        write_log("DELETE: %s - %s.%03d", delrange, path + 1, deleted - 1);
```
Hasil log:   
![Log Delete](https://github.com/Rkaaa404/Sisop-4-2025-IT35/blob/main/assets/deleteLog.png)
### Nomor 3 (Rayka)   
### Docker Init    
```Dockerfile
FROM ubuntu:20.04

RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y \
    fuse \
    gcc \
    libfuse-dev \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*

COPY antink.c .

RUN gcc -Wall `pkg-config fuse --cflags` antink.c -o antink `pkg-config fuse --libs`

CMD ["./antink", "-f", "./antink_mount"]
```
Ketika docker dijalankan maka program FUSE antink akan dicompile dan seterusnya, dengan dijalankan secara foreground
```docker-compose.yml
version: "3.8"

services:
  antink-fuse:
    build: .
    cap_add:
      - SYS_ADMIN
    devices:
      - /dev/fuse
    security_opt:
      - apparmor:unconfined
    volumes:
      - type: bind
        source: ./it24_host
        target: /it24_host           # Store Original File
      - type: bind
        source: ./antink_logs
        target: /antink_logs         # Store Log
      - type: volume
        source: antink_mount_vol
        target: /antink_mount        # Mount Point (FUSE, tidak perlu host lihat)
    tty: true
    stdin_open: true

volumes:
  antink_mount_vol:

```
Pemberlakuan 3 directory, dengan *antink_mount* berupa volume untuk penyimpanan file FUSE, sedangkan *it24_host* menjadi sumber file yang dibaca serta *antink_logs* menajdi tempat menyimpan log, dimana keduanga berupa bind mount.
Tree sebelum dilakukan penjalanan program:    
![Soal 3 Tree](https://github.com/Rkaaa404/Sisop-4-2025-IT35/blob/main/assets/soal3Tree.png)
### Filename detection and reversing 
```
### Filename detection and reversing
```c
int is_dangerous(const char *name) {
    return strstr(name, "nafis") || strstr(name, "kimcun");
}
void reverse_name(const char *src, char *dest) {
    size_t len = strlen(src);
    for (size_t i = 0; i < len; i++)
        dest[i] = src[len - 1 - i];
    dest[len] = '\0';
}
```
Fungsi pertama berfungsi mengembalikan nilai 1 jika file memiliki nama 'Kimcun' atau 'Nafis' dan 0 jika tidak ada. Sedangkan fungsi selanjutnya berguna untuk membalikkan namafile. Lalu impementasi fungsi ini terdapat pada:
```c
static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                       off_t offset, struct fuse_file_info *fi) {
    DIR *dp;
    struct dirent *de;

    char fullpath[MAX_PATH];
    snprintf(fullpath, sizeof(fullpath), "%s%s", base_dir, path);

    dp = opendir(fullpath);
    if (dp == NULL)
        return -errno;

    while ((de = readdir(dp)) != NULL) {
        char shown_name[NAME_MAX];
        if (is_dangerous(de->d_name)) {
            reverse_name(de->d_name, shown_name);
            log_activity("ALERT", de->d_name);
            log_activity("REVERSE", de->d_name);
        } else {
            strcpy(shown_name, de->d_name);
        }

```
Ketika pembacaan directory dan ditemukan file dengan nama berbahaya ('Nafis' atau 'Kimcun') maka akan melakukan reverse nama file serta mengirim log. Contoh hasil reversing nama file:    
![Soal 3 Mounted Tree](https://github.com/Rkaaa404/Sisop-4-2025-IT35/blob/main/assets/soal3MountDir.png)
### Normal txt file text ROT13
```c
void apply_rot13(char *buf, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if ('a' <= buf[i] && buf[i] <= 'z')
            buf[i] = 'a' + (buf[i] - 'a' + 13) % 26;
        else if ('A' <= buf[i] && buf[i] <= 'Z')
            buf[i] = 'A' + (buf[i] - 'A' + 13) % 26;
    }
}
```
Fungsi yang melakuka ROT13 pada string
```c
static int xmp_read(const char *path, char *buf, size_t size, off_t offset,
                    struct fuse_file_info *fi) {
    int fd;
    int res;

    char fullpath[MAX_PATH];
    snprintf(fullpath, sizeof(fullpath), "%s%s", base_dir, path);

    fd = open(fullpath, O_RDONLY);
    if (fd == -1)
        return -errno;

    res = pread(fd, buf, size, offset);
    if (res == -1) {
        res = -errno;
    } else if (strstr(path, ".txt") && !is_dangerous(path)) {
        apply_rot13(buf, res);
        log_activity("ENCRYPT", path);
    }

    close(fd);
    return res;
}
```
Melakukan pengecekan apakah file yang dibuka memiliki ekstensi txt dan bukan nama file yang berbahaya, jika iya maka akan melakukan ROT13 dan menaruh log ENCRYPT. Berikut ilustrasin dari ROT13 pada sistem FUSE:   
Sebelum:    
![Sebelum ROT13](https://github.com/Rkaaa404/Sisop-4-2025-IT35/blob/main/assets/soal3txt.png)    
Sesudah:    
![Sesudah ROT13](https://github.com/Rkaaa404/Sisop-4-2025-IT35/blob/main/assets/soal3ROT13.png)    

### Logger
```c
#define LOG_PATH "./antink_logs/it24.log"
char log_path[] = LOG_PATH;

void log_activity(const char *type, const char *path) {
    FILE *logf = fopen(log_path, "a");
    if (logf) {
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        fprintf(logf, "[%04d-%02d-%02d %02d:%02d:%02d] [%s] %s\n",
                t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
                t->tm_hour, t->tm_min, t->tm_sec,
                type, path);
        fclose(logf);
    }
}
```
Fungsi log_activity menerima menulis log di *./antink_logs/it24.log* secara append (terus bertambah) dan menerima char type untuk mendapatkan type operasi yang dilakukan serta char path yaitu path dari file yang terlibat.
Seperti pada kasus terjadinya log:
- Ditemukan file Nafis atau Kimcun:
```c
while ((de = readdir(dp)) != NULL) {
        char shown_name[NAME_MAX];
        if (is_dangerous(de->d_name)) {
            reverse_name(de->d_name, shown_name);
            log_activity("ALERT", de->d_name);
            log_activity("REVERSE", de->d_name);
        } else {
            strcpy(shown_name, de->d_name);
        }
```
Dimana akan mengirim type "ALERT" dan path berupa nama file
- Membaca isi file normal txt:
```c
else if (strstr(path, ".txt") && !is_dangerous(path)) {
        apply_rot13(buf, res);
        log_activity("ENCRYPT", path);
    }
```
Ketika dilakukan read file yang tidak berbahaya (tidak mengandung nafis dan kimcun) maka akan melakukan ROT_13 pada saat cat (dibaca) dan memberi log encrypt.

Hasil Log:     
![Hasil Log](https://github.com/Rkaaa404/Sisop-4-2025-IT35/blob/main/assets/soal3Log.png)
### File changes only in container
Agar perubahan file hanya terjadi dalam container, maka docker menggunakan volume untuk mount_dir:
```docker-compose.yml
- type: volume
        source: antink_mount_vol
        target: /antink_mount        # Mount Point (FUSE, tidak perlu host lihat)
    tty: true
    stdin_open: true

volumes:
  antink_mount_vol:
```
### Nomor 4 (Aria)
