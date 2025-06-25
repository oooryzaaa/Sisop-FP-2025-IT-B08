# Sisop-FP-2025-IT-B08

## Final Project Sistem Operasi 2025

### Kelompok B08
| Nama                        | NRP         |
|-----------------------------|-------------|
| Ahmad Ibnu Athaillah        | 5027241024  |
| Oryza Qiara Ramadhani       | 5027241084  |
| Putri Joselina Silitonga    | 5027241116  |
| Ananda Fitri Wibowo         | 5027241057  |

---

## Deskripsi Soal
**FUSE - Count characters:**
Buatlah sebuah program FUSE yang dapat mount sebuah directory. Saat sebuah file text di directory tersebut dibuka, maka setiap karakter yang ada di dalam file text tersebut akan dihitung jumlahnya dan dicatat di dalam sebuah file log bernama `count.log`. Sertakan juga tanggal dan waktu untuk tiap log (format bebas, asalkan masih bisa dibaca).

---

## Catatan Pengerjaan
- Program diimplementasikan menggunakan bahasa C dan library FUSE.
- Proses pembacaan file dioptimalkan dengan buffered read (per blok, bukan seluruh file sekaligus).
- Logging menggunakan file locking agar aman dari race condition.
- Analisis karakter dilakukan langsung dari file descriptor hasil open, tanpa membuka file dua kali.
- Semua source code, script, dan file terkait diletakkan di repository ini.
- Jika ada kendala atau soal yang tidak dapat diselesaikan, akan dicatat di bagian ini. (Saat ini: semua soal dapat diselesaikan)

---

## Struktur Repository
```
.
├── Bismillah/
│   └── fuse.c
├── README.md
...
```

---

## Penjelasan Solusi
### Teori FUSE
Filesystem in Userspace (FUSE) adalah arsitektur yang memungkinkan pengguna membuat virtual filesystem tanpa perlu menulis modul kernel. FUSE terdiri dari:
- **Kernel module:** menerima permintaan sistem file melalui VFS.
- **Daemon userspace:** menangani callback seperti getattr, readdir, open, dan read.

Pada program ini, FUSE digunakan untuk intercept proses pembukaan file teks dan menghitung jumlah karakter yang ada.

### Implementasi
- Fungsi utama FUSE yang digunakan:
  - `getattr`, `readdir`, `open`, `read`, `release`.
- Pada fungsi `open`, jika file yang dibuka adalah file teks, maka karakter di dalam file dihitung dan dicatat ke `count.log`.
- Proses pembacaan file dilakukan secara bertahap (buffered) agar efisien untuk file besar.
- Logging dilakukan dengan file lock agar tidak terjadi race condition.

### Eksekusi Loop Utama FUSE
```c
static struct fuse_operations operations = {
    .getattr = char_counter_getattr,
    .readdir = char_counter_readdir,
    .open    = char_counter_open,
    .read    = char_counter_read,
    .release = char_counter_release,
};
return fuse_main(argc - 1, fuse_argv, &operations, NULL);
```

### Cara Menjalankan
1. **Build:**
   ```bash
   gcc -Wall Bismillah/fuse.c `pkg-config fuse --cflags --libs` -o fuse_counter
   ```
2. **Jalankan:**
   ```bash
   ./fuse_counter <source_dir> <mount_point>
   ```
3. **Akses file teks di mount point, cek log di `<source_dir>/count.log`.**

### Video Menjalankan Program
[Demo Video (dummy link)](https://youtu.be/dummy-link)

---

## Daftar Pustaka
- Singh, A. (2014). Writing a Simple Filesystem Using FUSE in C. [Link](https://www.cs.nmsu.edu/~pfeiffer/classes/474/notes/FUSE/Fuse-tutorial.pdf)
- libfuse Project. (2021). FUSE: Filesystem in Userspace - Struct fuse_operations Documentation. [Link](https://libfuse.github.io/doxygen/structfuse__operations.html)
- Szeredi, M. (2007). Writing a FUSE Filesystem: a Tutorial. [Link](https://github.com/libfuse/libfuse/wiki/Writing-Your-Own-Filesystem)
- Stack Overflow
