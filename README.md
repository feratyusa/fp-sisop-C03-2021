## Final Project C03 SISOP 2021

- Ega Prabu Pamungkas (05111940000014)
- A. Zidan Abdillah Majid (05111940000070)
- Jundullah Hanif Robbani (05111940000144)

----------------------------------------------
Pastikan server berjalan dahulu sebelum client

### Server
1. Compile gcc server.c -o [nama output]
2. Sudah (karena daemon jadi akan berjalan di background)
3. ps -aux | grep [nama output] untuk melihat apakah sudah berjalan

Untuk membaca data kiriman dari client
```c
      memset(buf, '\0', DATA_BUFFER);
      ret_val =  recv(all_connections[i], buf, DATA_BUFFER, 0);
```
Untuk mengirim data ke client
```c
    ret_val = send(all_connections[i], buf, sizeof(buf), 0);
```
### Client
1. Compile
2. Sudah

Untuk mengirim data ke server dan menerima data dari server
 ```c
        ret_val = send(fd, message, sizeof(message), 0);
        ret_val = recv(fd, message, sizeof(message), 0);
```
