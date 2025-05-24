# Process-Thread-Simulasyonu

Bu proje, 10 katlı bir apartmanın inşaat sürecini simüle eden bir C programıdır. İnşaat süreci, temel atma, kat inşaatı, daire iç dizaynı ve çatı inşaatı gibi aşamaları içerir. Program, çoklu süreçler (`fork`) ve iş parçacıkları (`pthread`) kullanarak paralel işlemleri simüle eder, paylaşılan bellek (`mmap`) ile maliyet hesaplamalarını takip eder ve senkronizasyon mekanizmaları (mutex ve semaforlar) ile veri tutarlılığını sağlar.

## Özellikler

-   **Çoklu Süreç ve İş Parçacığı**: Kat inşaatları için süreçler (`fork`), daire iç dizaynları için iş parçacıkları (`pthread`) kullanılır.
-   **Paylaşılan Bellek**: Maliyet verileri, süreçler ve iş parçacıkları arasında paylaşılır.
-   **Senkronizasyon**: Semaforlar ile kat inşaatları sırayla yapılır; mutex'ler ile maliyet güncellemeleri senkronize edilir.
-   **Rastgele Gecikmeler**: İnşaat süreçlerini gerçekçi bir şekilde simüle etmek için rastgele bekleme süreleri kullanılır.
-   **Renkli Konsol Çıktıları**: ANSI renk kodları ile okunabilir konsol çıktıları sağlanır.
-   **Maliyet Takibi**: Her işlem için malzeme ve işçilik maliyetleri ayrı ayrı hesaplanır ve detaylı bir maliyet özeti sunulur.

## Gereksinimler

Bu programı derlemek ve çalıştırmak için aşağıdaki bağımlılıklara ihtiyaç vardır:

-   **C Derleyicisi**: GCC veya Clang gibi POSIX uyumlu bir C derleyicisi.
-   **Kütüphaneler**:
    -   POSIX iş parçacıkları (`pthread`)
    -   Paylaşılan bellek (`sys/mman.h`)
    -   Semaforlar (`semaphore.h`)
    -   POSIX sistem çağrıları (`unistd.h`, `sys/wait.h`, `fcntl.h`)
-   **İşletim Sistemi**: Linux veya POSIX uyumlu bir sistem (örneğin, Ubuntu, Debian). Windows üzerinde çalışmaz.

## Kurulum ve Çalıştırma

1.  **Depoyu Klonlayın**:
    
    ```bash
    git clone https://github.com/belerumut/Process-Thread-Simulasyonu.git
    
    ```
    
    Klonlama işleminden sonra, dosyalar doğrudan repo kök dizininde yer alır.
    
2.  **Programı Derleyin**:  
    Programı derlemek için aşağıdaki komutu kullanın:
    
    ```bash
    gcc -o Process-Thread-Simulasyonu Process-Thread-Simulasyonu.c -pthread -lrt
    
    ```
    
    -   `-pthread`: POSIX iş parçacıkları kütüphanesini bağlar.
    -   `-lrt`: Gerçek zamanlı kütüphaneleri (semaforlar için) bağlar.
3.  **Programı Çalıştırın**:  
    Derlenen programı çalıştırın:
    
    ```bash
    ./Process-Thread-Simulasyonu
    
    ```
    

## Programın Çalışma Mantığı

Program, bir apartman inşaatını şu aşamalarda simüle eder:

1.  **Temel Atma**: Zemin etüdü, kazı, beton dökümü ve kürleme işlemleri.
2.  **Kat İnşaatı**: Her kat için kaba inşaat (temel yapı, kolonlar, duvarlar, koridor ve merdivenler) ayrı bir süreçte yapılır.
3.  **Daire İç Dizaynı**: Her katta bulunan 4 daire için elektrik, sıhhi tesisat, boya, zemin kaplama, pencere montajı, mutfak dolabı, banyo montajı ve son temizlik işlemleri paralel olarak iş parçacıklarıyla gerçekleştirilir.
4.  **Çatı ve Ortak Alanlar**: Çatı iskeleti, çatı yapımı, asansör sistemleri ve ortak alan dekorasyonu.

### Senkronizasyon

-   **Semaforlar**: Kat inşaatları sırayla yapılır; bir kat tamamlanmadan bir sonraki katın inşaatı başlamaz.
-   **Mutex'ler**: Maliyet güncellemeleri için paylaşılan bellekte bir mutex (`cost_mutex`) kullanılır. Ayrıca, elektrik, sıhhi tesisat ve boya işlemleri için ayrı mutex'ler vardır.
-   **Asansör Semaforu**: Mutfak dolabı malzemelerinin taşınması için asansör erişimi senkronize edilir.

### Maliyet Takibi

Her işlem için malzeme ve işçilik maliyetleri hesaplanır ve kategorilere göre saklanır (örneğin, temel, kat, elektrik, vb.). Program sonunda detaylı bir maliyet özeti sunulur.

## Örnek Çıktı

Program çalıştırıldığında, aşağıdaki gibi bir çıktı üretir:

```
10 KATLI APARTMAN İNŞAAT SIMÜLASYONU BAŞLIYOR
Toplam süre: 365 gün
====================================
=== TEMEL ATMA İŞLEMLERİ BAŞLADI ===
Zemin etüdü yapılıyor...
Temel kazısı başladı...
...
=== GÜN 1 === (Toplam Maliyet: 210000.00 TL)
=== KAT 1 KABA İNŞAATI BAŞLADI (PID: 12345) ===
...
Kategori: floor, Malzeme maliyeti: 10000.00 TL, İşçilik maliyeti: 4000.00 TL, Yeni toplam: 214000.00 TL
...
=== İNŞAAT TAMAMLANDI ===
Toplam süre: 54 gün
Toplam maliyet: 1234567.89 TL
Kat başına ortalama maliyet: 123456.79 TL
Daire başına ortalama maliyet: 30864.20 TL
=== MALİYET ÖZETİ ===
...

```
