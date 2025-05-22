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
  a. Relics File Reading  
  b. Mount_dir file manipulation  
  c. New file in mount_dir chunked in relics
  d. Mount_dir file complete deletion
  e. FUSE activity log
- [Nomor 3](#nomor-3-rayka)
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
