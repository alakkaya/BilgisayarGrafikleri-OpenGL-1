# 📋 Bilgisayar Grafikleri - Vize Öncesi Proje 

Bu sahne, yıllardır yaşadığım ve hayatımın yaklaşık 6 senesini yani odamı temsil ediyor. Masanın üzerinde bir monitör ve hemen yanında bir bilgisayar kasası bulunuyor. Masanın sağ tarafında ise bir yatak yer alıyor. Bu oda, sınavlara hazırlandığım, projeler üzerinde çalıştığım ve hayallerimi şekillendirdiğim bir alanı temsil ediyor. Buradaki her bir nesne, bu odanın bir parçasını ve aslında benim hayat mücadelemi anlatıyor.

## 📑 Proje Raporu
Proje raporuna [BG-Rapor1.pdf](./BG-Rapor1.pdf) bağlantısından ulaşabilirsiniz.

## 🎬 Tanıtım Videosu

## 🛠 Kullanılan Kütüphaneler (Gereklilikler)

Projenin çalışabilmesi için aşağıdaki kütüphanelerin indirilmesi zorunludur.

- GLFW: 3.3 veya üstü (Pencere oluşturma ve giriş işlemleri için)
- GLEW: 2.1.0 veya üstü (OpenGL fonksiyonlarının yüklenmesi için)
- GLM: 0.9.9.8 veya üstü (Matematiksel işlemler, matris ve vektör hesaplamaları için)
- OpenGL: 3.3 veya üstü (Grafik API'si)
- GLSL (OpenGL Shading Language): 3.30 (Shader programlama için)

## ✅ Uygulamayı Çalıştırma Adımları

1. Proje dosyalarını indirin veya klonlayın:
     ```bash
     git clone https://github.com/alakkaya/BilgisayarGrafikleriProje1.git
     cd BilgisayarGrafikleriProje1
     ```
2. Gerekli kütühaneleri yükleyin (GLFW, GLEW)
3. Projeyi derlemek için herhangi hata çıkmaması için aşağıdaki kodu terminale yapıştırın.
     ```bash
      clang++ -std=c++17 -fdiagnostics-color=always -Wall -g \
      -I./dependencies/include \
      -L./dependencies/library \
      ./dependencies/library/libglfw.3.4.dylib \
      ./dependencies/library/libGLEW.2.2.0.dylib \
      ./main.cpp ./glad.c \
      -o app \
      -framework OpenGL \
      -framework Cocoa \
      -framework IOKit \
      -framework CoreVideo \
      -framework CoreFoundation \
      -Wno-deprecated
     ```
4. Çalıştırmak için terminale aşağıdaki komutu yazın.
    ```bash
      ./app
     ```

## 🕹️ Kullanım

- `W/A/S/D/LSHIFT/LCTRL` ile kamera etkileşimine geçebilirsiniz.
- `Mouse` ile özgürce kameranızı döndürebilirsiniz


## 🖼️ Görseller

<table>
        <tr>
            <th>Ön Görünüm</th>
        </tr>
        <tr>
            <td><img src="https://github.com/user-attachments/assets/25e64573-b54b-40c7-b090-8d9a3a3fc313" ></td>
        </tr>
        <tr>
            <th>Üst Görünüm</th>
        </tr>
        <tr>
            <td><img src="https://github.com/user-attachments/assets/a4a89f6a-ff82-4a27-a920-c3989cd9a5df" ></td>
        </tr>
        <tr>
            <th>Çapraz Görünüm</th>
        </tr>
        <tr>
            <td><img src="https://github.com/user-attachments/assets/95858b77-0486-480f-b6b9-bad82a717d57" ></td>
        </tr>
    </table>



