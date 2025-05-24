// Gerekli kütüphaneleri dahil ediyoruz
#include <stdio.h>         // Standart giriş/çıkış işlemleri için
#include <stdlib.h>        // Dinamik bellek yönetimi ve çıkış fonksiyonları için
#include <unistd.h>        // POSIX sistem çağrıları (fork, getpid vb.) için
#include <sys/wait.h>      // Süreç bekleme fonksiyonları için
#include <pthread.h>       // Thread işlemleri için
#include <semaphore.h>     // Semaforlar için
#include <time.h>          // Zamanla ilgili fonksiyonlar (rand, nanosleep) için
#include <fcntl.h>         // Dosya kontrol işlemleri için (paylaşılan bellek)
#include <sys/mman.h>      // Bellek yönetimi (mmap) için
#include <string.h>        // strcmp gibi string işlemleri için

// Apartman yapılandırması için sabitler
#define TOTAL_FLOORS 10           // Toplam kat sayısı
#define APARTMENTS_PER_FLOOR 4    // Her katta bulunan daire sayısı
#define TOTAL_DAYS 365            // Simülasyonun toplam gün sayısı

// Renkli konsol çıktıları için ANSI renk kodları
#define COLOR_RED     "\033[1;31m"  // Kırmızı renk
#define COLOR_GREEN   "\033[1;32m"  // Yeşil renk
#define COLOR_YELLOW  "\033[1;33m"  // Sarı renk
#define COLOR_BLUE    "\033[1;34m"  // Mavi renk
#define COLOR_MAGENTA "\033[1;35m"  // Magenta renk
#define COLOR_CYAN    "\033[1;36m"  // Cyan renk
#define COLOR_RESET   "\033[0m"     // Renk sıfırlama

// Paylaşılan bellekte maliyetleri saklamak için struct yapısı
struct SharedData {
    float total_cost;                 // Toplam inşaat maliyeti
    float foundation_material_cost;    // Temel için malzeme maliyeti
    float foundation_labor_cost;      // Temel için işçilik maliyeti
    float floor_material_cost;        // Kat inşaatı için malzeme maliyeti
    float floor_labor_cost;           // Kat inşaatı için işçilik maliyeti
    float electrical_material_cost;   // Elektrik tesisatı malzeme maliyeti
    float electrical_labor_cost;      // Elektrik tesisatı işçilik maliyeti
    float plumbing_material_cost;     // Sıhhi tesisat malzeme maliyeti
    float plumbing_labor_cost;        // Sıhhi tesisat işçilik maliyeti
    float painting_material_cost;     // Boya malzeme maliyeti
    float painting_labor_cost;        // Boya işçilik maliyeti
    float flooring_material_cost;     // Zemin kaplama malzeme maliyeti
    float flooring_labor_cost;        // Zemin kaplama işçilik maliyeti
    float window_material_cost;       // Pencere montajı malzeme maliyeti
    float window_labor_cost;          // Pencere montajı işçilik maliyeti
    float kitchen_material_cost;      // Mutfak dolabı malzeme maliyeti
    float kitchen_labor_cost;         // Mutfak dolabı işçilik maliyeti
    float bathroom_material_cost;     // Banyo montajı malzeme maliyeti
    float bathroom_labor_cost;        // Banyo montajı işçilik maliyeti
    float cleaning_material_cost;     // Son temizlik malzeme maliyeti
    float cleaning_labor_cost;        // Son temizlik işçilik maliyeti
    float roof_material_cost;         // Çatı ve ortak alanlar malzeme maliyeti
    float roof_labor_cost;            // Çatı ve ortak alanlar işçilik maliyeti
    pthread_mutex_t cost_mutex;       // Maliyet güncellemeleri için mutex
};

// Senkronizasyon nesneleri
sem_t *floor_semaphore[TOTAL_FLOORS]; // Her kat için semafor dizisi
sem_t elevator_sem;                    // Asansör erişimi için semafor
sem_t stairs_sem;                     // Merdiven erişimi için semafor

// Global değişkenler
struct SharedData *shared_data; // Paylaşılan bellek pointer'ı
int construction_day = 1;       // Simülasyonda geçerli gün sayısı

// Rastgele gecikme fonksiyonu: İnşaat süreçlerini simüle etmek için rastgele bekleme süresi
void random_delay(double min_sec, double max_sec) {
    double sec = min_sec + (rand() / (double)RAND_MAX) * (max_sec - min_sec); // Rastgele süre hesapla
    struct timespec ts = {
        .tv_sec = (time_t)sec,                                                // Saniye cinsinden tam sayı kısmı
        .tv_nsec = (long)((sec - (time_t)sec) * 1e9)                          // Nanosaniye cinsinden ondalık kısmı
    };
    nanosleep(&ts, NULL); // Belirtilen süre kadar bekle
}

// Maliyet ekleme fonksiyonu: Malzeme ve işçilik maliyetlerini paylaşılan belleğe kaydeder
void add_cost(float material_cost, float labor_cost, char* category) {
    pthread_mutex_lock(&shared_data->cost_mutex);            // Maliyet güncellemesi için mutex kilitle
    shared_data->total_cost += (material_cost + labor_cost); // Toplam maliyete ekle

    // Kategoriye göre ilgili maliyet alanlarını güncelleme kısmı
    if (strcmp(category, "foundation") == 0) {
        shared_data->foundation_material_cost += material_cost;
        shared_data->foundation_labor_cost += labor_cost;
    } else if (strcmp(category, "floor") == 0) {
        shared_data->floor_material_cost += material_cost;
        shared_data->floor_labor_cost += labor_cost;
    } else if (strcmp(category, "electrical") == 0) {
        shared_data->electrical_material_cost += material_cost;
        shared_data->electrical_labor_cost += labor_cost;
    } else if (strcmp(category, "plumbing") == 0) {
        shared_data->plumbing_material_cost += material_cost;
        shared_data->plumbing_labor_cost += labor_cost;
    } else if (strcmp(category, "painting") == 0) {
        shared_data->painting_material_cost += material_cost;
        shared_data->painting_labor_cost += labor_cost;
    } else if (strcmp(category, "flooring") == 0) {
        shared_data->flooring_material_cost += material_cost;
        shared_data->flooring_labor_cost += labor_cost;
    } else if (strcmp(category, "window") == 0) {
        shared_data->window_material_cost += material_cost;
        shared_data->window_labor_cost += labor_cost;
    } else if (strcmp(category, "kitchen") == 0) {
        shared_data->kitchen_material_cost += material_cost;
        shared_data->kitchen_labor_cost += labor_cost;
    } else if (strcmp(category, "bathroom") == 0) {
        shared_data->bathroom_material_cost += material_cost;
        shared_data->bathroom_labor_cost += labor_cost;
    } else if (strcmp(category, "cleaning") == 0) {
        shared_data->cleaning_material_cost += material_cost;
        shared_data->cleaning_labor_cost += labor_cost;
    } else if (strcmp(category, "roof") == 0) {
        shared_data->roof_material_cost += material_cost;
        shared_data->roof_labor_cost += labor_cost;
    }

    // Maliyet güncellemesini konsola yazdırma
    printf("Kategori: %s, Malzeme maliyeti: %.2f TL, İşçilik maliyeti: %.2f TL, Yeni toplam: %.2f TL\n",
           category, material_cost, labor_cost, shared_data->total_cost);
    pthread_mutex_unlock(&shared_data->cost_mutex); // Mutex kilidini aç
}

// Gün bilgisini ve o güne kadar olan toplam maliyeti yazdıran fonksiyon
void print_day_info() {
    printf(COLOR_CYAN "\n=== GÜN %d === (Toplam Maliyet: %.2f TL)\n" COLOR_RESET,
           construction_day++, shared_data->total_cost); // Gün sayısını artır ve toplam maliyeti yaz
}

// Daire işlemleri için gerekli fonksiyonlar
// Elektrik tesisatı fonksiyonu
void electrical_work(int floor, int apartment) {
    static pthread_mutex_t electric_mutex = PTHREAD_MUTEX_INITIALIZER; // Elektrik işlemleri için statik mutex
    pthread_mutex_lock(&electric_mutex);                               // Aynı anda tek elektrik işlemi yapılması sağlanır
    printf(COLOR_YELLOW "[Kat %d Daire %d] Elektrik tesisatı yapılıyor...\n" COLOR_RESET, floor+1, apartment+1);
    random_delay(0.3, 0.7);                                            // Rastgele gecikme ile işlemi simüle et
    printf(COLOR_YELLOW "[Kat %d Daire %d] Elektrik tesisatı tamamlandı.\n" COLOR_RESET, floor+1, apartment+1);
    pthread_mutex_unlock(&electric_mutex);                             // Mutex kilidini aç
    float material_cost = 3000.0f;                                     // Malzeme maliyeti
    float labor_cost = material_cost * 0.4;                            // İşçilik maliyeti (malzeme maaliyetinin %40)
    add_cost(material_cost, labor_cost, "electrical");                 // Maliyeti ekle
}

// Sıhhi tesisat fonksiyonu
void plumbing_work(int floor, int apartment) {
    static pthread_mutex_t plumbing_mutex = PTHREAD_MUTEX_INITIALIZER; // Sıhhi tesisat için statik mutex
    pthread_mutex_lock(&plumbing_mutex);                               // Aynı anda tek sıhhi tesisat işlemi yapılması sağlanır
    printf(COLOR_BLUE "[Kat %d Daire %d] Sıhhi tesisat yapılıyor...\n" COLOR_RESET, floor+1, apartment+1);
    random_delay(0.3, 0.6);                                            // Rastgele gecikme
    printf(COLOR_BLUE "[Kat %d Daire %d] Sıhhi tesisat tamamlandı.\n" COLOR_RESET, floor+1, apartment+1);
    pthread_mutex_unlock(&plumbing_mutex); // Mutex kilidini aç
    float material_cost = 2000.0f;                                     // Malzeme maliyeti
    float labor_cost = material_cost * 0.4;                            // İşçilik maliyeti
    add_cost(material_cost, labor_cost, "plumbing");                   // Maliyeti ekle
}

// Boya fonksiyonu
void painting_work(int floor, int apartment) {
    static pthread_mutex_t paint_mutex = PTHREAD_MUTEX_INITIALIZER; // Boya için statik mutex
    pthread_mutex_lock(&paint_mutex);                               // Aynı anda tek boya işlemi yapılması sağlanır
    printf(COLOR_MAGENTA "[Kat %d Daire %d] Boya yapılıyor...\n" COLOR_RESET, floor+1, apartment+1);
    random_delay(0.5, 1.0);                                         // Rastgele gecikme
    printf(COLOR_MAGENTA "[Kat %d Daire %d] Boya tamamlandı.\n" COLOR_RESET, floor+1, apartment+1);
    pthread_mutex_unlock(&paint_mutex);                             // Mutex kilidini aç
    float material_cost = 1500.0f;                                  // Malzeme maliyeti
    float labor_cost = material_cost * 0.4;                         // İş последние
    add_cost(material_cost, labor_cost, "painting");                // Maliyeti ekle
}

// Zemin kaplama fonksiyonu
void flooring_work(int floor, int apartment) {
    printf(COLOR_GREEN "[Kat %d Daire %d] Zemin kaplaması yapılıyor...\n" COLOR_RESET, floor+1, apartment+1);
    random_delay(0.5, 1.0);                                         // Rastgele gecikme
    printf(COLOR_GREEN "[Kat %d Daire %d] Zemin kaplaması tamamlandı.\n" COLOR_RESET, floor+1, apartment+1);
    float material_cost = 1000.0f;                                  // Malzeme maliyeti
    float labor_cost = material_cost * 0.4;                         // İşçilik maliyeti
    add_cost(material_cost, labor_cost, "flooring");                // Maliyeti ekle
}

// Pencere montajı fonksiyonu
void window_installation(int floor, int apartment) {
    printf("[Kat %d Daire %d] Pencere montajı yapılıyor...\n", floor+1, apartment+1);
    random_delay(0.4, 0.9);                                         // Rastgele gecikme
    printf("[Kat %d Daire %d] Pencere montajı tamamlandı.\n", floor+1, apartment+1);
    float material_cost = 500.0f;                                   // Malzeme maliyeti
    float labor_cost = material_cost * 0.4;                         // İşçilik maliyeti
    add_cost(material_cost, labor_cost, "window");                  // Maliyeti ekle
}

// Mutfak dolabı montajı fonksiyonu
void kitchen_cabinet(int floor, int apartment) {
    sem_wait(&elevator_sem);                                        // Asansör erişimi için semafor bekle
    printf(COLOR_RED "[Kat %d Daire %d] Mutfak dolabı malzemesi asansörle taşınıyor...\n" COLOR_RESET, floor+1, apartment+1);
    random_delay(0.2, 0.4);                                         // Asansör taşıma süresi
    sem_post(&elevator_sem);                                        // Asansör semaforunu serbest bırak

    printf(COLOR_RED "[Kat %d Daire %d] Mutfak dolabı montajı yapılıyor...\n" COLOR_RESET, floor+1, apartment+1);
    random_delay(0.5, 1.0);                                         // Montaj süresi
    printf(COLOR_RED "[Kat %d Daire %d] Mutfak dolabı montajı tamamlandı.\n" COLOR_RESET, floor+1, apartment+1);
    float material_cost = 5000.0f;                                  // Malzeme maliyeti
    float labor_cost = material_cost * 0.4;                         // İşçilik maliyeti
    add_cost(material_cost, labor_cost, "kitchen");                 // Maliyeti ekle
}

// Banyo montajı fonksiyonu
void bathroom_installation(int floor, int apartment) {
    printf(COLOR_YELLOW "[Kat %d Daire %d] Banyo montajı yapılıyor...\n" COLOR_RESET, floor+1, apartment+1);
    random_delay(0.5, 1.0);                                         // Rastgele gecikme
    printf(COLOR_YELLOW "[Kat %d Daire %d] Banyo montajı tamamlandı.\n" COLOR_RESET, floor+1, apartment+1);
    float material_cost = 4000.0f;                                  // Malzeme maliyeti
    float labor_cost = material_cost * 0.4;                         // İşçilik maliyeti
    add_cost(material_cost, labor_cost, "bathroom");                // Maliyeti ekle
}

// Son temizlik fonksiyonu
void final_cleaning(int floor, int apartment) {
    printf("[Kat %d Daire %d] Son temizlik yapılıyor...\n", floor+1, apartment+1);
    random_delay(0.2, 0.5);                                         // Rastgele gecikme
    printf("[Kat %d Daire %d] Son temizlik tamamlandı.\n", floor+1, apartment+1);
    float material_cost = 300.0f;                                   // Malzeme maliyeti
    float labor_cost = material_cost * 0.4;                         // İşçilik maliyeti
    add_cost(material_cost, labor_cost, "cleaning");                // Maliyeti ekle
}

// Çatı iskeleti fonksiyonu
void roof_skeleton() {
    printf(COLOR_RED "[Çatı] İskelet yapımı devam ediyor...\n" COLOR_RESET);
    random_delay(2.0, 3.0);                                         // Rastgele gecikme
    float material_cost = 6000.0f * TOTAL_FLOORS;                   // Çatı maliyeti (kat sayısına bağlı)
    float labor_cost = material_cost * 0.4;                         // İşçilik maliyeti
    add_cost(material_cost, labor_cost, "roof");                    // Maliyeti ekle
    printf(COLOR_RED "[Çatı] İskelet yapımı tamamlandı.\n" COLOR_RESET);
}

// Daire inşaatı için thread fonksiyonu
void* apartment_construction(void* arg) {
    int floor = ((int*)arg)[0];                                     // Kat numarasını al
    int apartment = ((int*)arg)[1];                                 // Daire numarasını al
    free(arg);                                                      // Dinamik olarak ayrılan argüman belleğini serbest bırak

    printf("\n" COLOR_CYAN "--- Kat %d Daire %d iç dizayn başladı ---\n" COLOR_RESET, floor+1, apartment+1);

    // Her dairenin iç dizaynı için tüm işlemler sırayla gerçekleştirilir
    electrical_work(floor, apartment);
    plumbing_work(floor, apartment);
    painting_work(floor, apartment);
    flooring_work(floor, apartment);
    window_installation(floor, apartment);
    kitchen_cabinet(floor, apartment);
    bathroom_installation(floor, apartment);
    final_cleaning(floor, apartment);

    printf(COLOR_CYAN "--- Kat %d Daire %d iç dizayn tamamlandı ---\n\n" COLOR_RESET, floor+1, apartment+1);
    return NULL;
}

// Kat inşaatı fonksiyonu (kaba inşaat ve ortak alanlar)
void floor_construction(int floor) {
    printf("\n" COLOR_YELLOW "=== KAT %d KABA İNŞAATI BAŞLADI (PID: %d) ===\n" COLOR_RESET, floor+1, getpid());

    // Temel yapı çalışmaları
    printf("[Kat %d] Temel yapı çalışmaları başladı...\n", floor+1);
    random_delay(1.0, 2.0);                             // Rastgele gecikme
    float material_cost1 = 10000.0f;                    // Malzeme maliyeti
    float labor_cost1 = material_cost1 * 0.4;           // İşçilik maliyeti
    add_cost(material_cost1, labor_cost1, "floor");     // Maliyeti ekle

    // Kolon ve kirişler
    printf("[Kat %d] Kolon ve kirişler yapılıyor...\n", floor+1);
    random_delay(1.5, 2.5);                             // Rastgele gecikme
    float material_cost2 = 15000.0f;                    // Malzeme maliyeti
    float labor_cost2 = material_cost2 * 0.4;           // İşçilik maliyeti
    add_cost(material_cost2, labor_cost2, "floor");     // Maliyeti ekle

    // Duvarlar
    printf("[Kat %d] Duvarlar örülüyor...\n", floor+1);
    random_delay(1.0, 2.0);                             // Rastgele gecikme
    float material_cost3 = 8000.0f;                     // Malzeme maliyeti
    float labor_cost3 = material_cost3 * 0.4;           // İşçilik maliyeti
    add_cost(material_cost3, labor_cost3, "floor");     // Maliyeti ekle

    // Koridor ve merdivenler
    printf("[Kat %d] Koridor ve merdivenler yapılıyor...\n", floor+1);
    random_delay(0.5, 1.0);                             // Rastgele gecikme
    float material_cost4 = 5000.0f;                     // Malzeme maliyeti
    float labor_cost4 = material_cost4 * 0.4;           // İşçilik maliyeti
    add_cost(material_cost4, labor_cost4, "floor");     // Maliyeti ekle

    printf(COLOR_YELLOW "=== KAT %d KABA İNŞAATI TAMAMLANDI ===\n\n" COLOR_RESET, floor+1);

    // Bir sonraki katın inşaatına izin vermek için semafor sinyali gönder
    if (floor < TOTAL_FLOORS - 1) {
        printf("Kat %d: Bir sonraki kat (%d) için semafor sinyal veriliyor\n", floor+1, floor+2);
        if (sem_post(floor_semaphore[floor+1]) == -1) {
            perror("sem_post failed"); // Hata kontrolü
        }
        usleep(1000); // 1 ms gecikme
    }
}

// Temel atma fonksiyonu
void build_foundation() {
    printf(COLOR_RED "\n=== TEMEL ATMA İŞLEMLERİ BAŞLADI ===\n" COLOR_RESET);

    printf("Zemin etüdü yapılıyor...\n");
    random_delay(1.0, 2.0); // Rastgele gecikme

    printf("Temel kazısı başladı...\n");
    random_delay(2.0, 3.0); // Rastgele gecikme

    printf("Temel betonu dökülüyor...\n");
    random_delay(1.5, 2.5); // Rastgele gecikme

    printf("Temel kürleniyor...\n");
    random_delay(3.0, 5.0); // Rastgele gecikme

    // İlk katın inşaatına izin vermek için semafor sinyali gönder
    printf("Temel: İlk kat için semafor sinyal veriliyor\n");
    if (sem_post(floor_semaphore[0]) == -1) {
        perror("sem_post failed"); // Hata kontrolü
    }

    float material_cost = 150000.0f;                    // Malzeme maliyeti
    float labor_cost = material_cost * 0.4;             // İşçilik maliyeti
    add_cost(material_cost, labor_cost, "foundation");  // Maliyeti ekle
    printf(COLOR_RED "=== TEMEL ATMA İŞLEMLERİ TAMAMLANDI ===\n\n" COLOR_RESET);
}

// Maliyet özetini yazdıran fonksiyon
void print_cost_summary() {
    printf(COLOR_CYAN "\n=== MALİYET ÖZETİ ===\n" COLOR_RESET);
    printf("Kategori                | Malzeme Maliyeti | İşçilik Maliyeti | Toplam\n");
    printf("------------------------|------------------|------------------|------------\n");
    // Her kategori için malzeme, işçilik ve toplam maliyetleri yazdır
    printf("Temel Atma             | %.2f TL         | %.2f TL         | %.2f TL\n",
           shared_data->foundation_material_cost, shared_data->foundation_labor_cost,
           shared_data->foundation_material_cost + shared_data->foundation_labor_cost);
    printf("Kat İnşaatı            | %.2f TL         | %.2f TL         | %.2f TL\n",
           shared_data->floor_material_cost, shared_data->floor_labor_cost,
           shared_data->floor_material_cost + shared_data->floor_labor_cost);
    printf("Elektrik Tesisatı      | %.2f TL         | %.2f TL         | %.2f TL\n",
           shared_data->electrical_material_cost, shared_data->electrical_labor_cost,
           shared_data->electrical_material_cost + shared_data->electrical_labor_cost);
    printf("Sıhhi Tesisat          | %.2f TL         | %.2f TL         | %.2f TL\n",
           shared_data->plumbing_material_cost, shared_data->plumbing_labor_cost,
           shared_data->plumbing_material_cost + shared_data->plumbing_labor_cost);
    printf("Boya                   | %.2f TL         | %.2f TL         | %.2f TL\n",
           shared_data->painting_material_cost, shared_data->painting_labor_cost,
           shared_data->painting_material_cost + shared_data->painting_labor_cost);
    printf("Zemin Kaplama          | %.2f TL         | %.2f TL         | %.2f TL\n",
           shared_data->flooring_material_cost, shared_data->flooring_labor_cost,
           shared_data->flooring_material_cost + shared_data->flooring_labor_cost);
    printf("Pencere Montajı        | %.2f TL         | %.2f TL         | %.2f TL\n",
           shared_data->window_material_cost, shared_data->window_labor_cost,
           shared_data->window_material_cost + shared_data->window_labor_cost);
    printf("Mutfak Dolabı          | %.2f TL         | %.2f TL         | %.2f TL\n",
           shared_data->kitchen_material_cost, shared_data->kitchen_labor_cost,
           shared_data->kitchen_material_cost + shared_data->kitchen_labor_cost);
    printf("Banyo Montajı          | %.2f TL         | %.2f TL         | %.2f TL\n",
           shared_data->bathroom_material_cost, shared_data->bathroom_labor_cost,
           shared_data->bathroom_material_cost + shared_data->bathroom_labor_cost);
    printf("Son Temizlik           | %.2f TL         | %.2f TL         | %.2f TL\n",
           shared_data->cleaning_material_cost, shared_data->cleaning_labor_cost,
           shared_data->cleaning_material_cost + shared_data->cleaning_labor_cost);
    printf("Çatı ve Ortak Alanlar  | %.2f TL         | %.2f TL         | %.2f TL\n",
           shared_data->roof_material_cost, shared_data->roof_labor_cost,
           shared_data->roof_material_cost + shared_data->roof_labor_cost);
    printf("------------------------|------------------|------------------|------------\n");

    // Toplam malzeme ve işçilik maliyetlerini hesaplama
    float total_material = shared_data->foundation_material_cost + shared_data->floor_material_cost +
                          shared_data->electrical_material_cost + shared_data->plumbing_material_cost +
                          shared_data->painting_material_cost + shared_data->flooring_material_cost +
                          shared_data->window_material_cost + shared_data->kitchen_material_cost +
                          shared_data->bathroom_material_cost + shared_data->cleaning_material_cost +
                          shared_data->roof_material_cost;
    float total_labor = shared_data->foundation_labor_cost + shared_data->floor_labor_cost +
                        shared_data->electrical_labor_cost + shared_data->plumbing_labor_cost +
                        shared_data->painting_labor_cost + shared_data->flooring_labor_cost +
                        shared_data->window_labor_cost + shared_data->kitchen_labor_cost +
                        shared_data->bathroom_labor_cost + shared_data->cleaning_labor_cost +
                        shared_data->roof_labor_cost;
    printf("TOPLAM                 | %.2f TL         | %.2f TL         | %.2f TL\n",
           total_material, total_labor, shared_data->total_cost);
}

int main() {
    srand(time(NULL)); // Rastgele sayı üreticisini başlat

    // Paylaşılan bellek oluştur
    int shm_fd = shm_open("/construction_shm", O_CREAT | O_RDWR, 0666); // Paylaşılan bellek dosyası oluştur
    if (shm_fd == -1) {
        perror("shm_open failed"); // Hata kontrolü
        exit(EXIT_FAILURE);
    }

    if (ftruncate(shm_fd, sizeof(struct SharedData)) == -1) { // Bellek boyutunu ayarla
        perror("ftruncate failed");
        exit(EXIT_FAILURE);
    }

    // Paylaşılan belleği eşle
    shared_data = mmap(NULL, sizeof(struct SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_data == MAP_FAILED) {
        perror("mmap failed");
        exit(EXIT_FAILURE);
    }

    // Paylaşılan bellek değişkenlerini sıfırla
    shared_data->total_cost = 0.0f;
    shared_data->foundation_material_cost = 0.0f;
    shared_data->foundation_labor_cost = 0.0f;
    shared_data->floor_material_cost = 0.0f;
    shared_data->floor_labor_cost = 0.0f;
    shared_data->electrical_material_cost = 0.0f;
    shared_data->electrical_labor_cost = 0.0f;
    shared_data->plumbing_material_cost = 0.0f;
    shared_data->plumbing_labor_cost = 0.0f;
    shared_data->painting_material_cost = 0.0f;
    shared_data->painting_labor_cost = 0.0f;
    shared_data->flooring_material_cost = 0.0f;
    shared_data->flooring_labor_cost = 0.0f;
    shared_data->window_material_cost = 0.0f;
    shared_data->window_labor_cost = 0.0f;
    shared_data->kitchen_material_cost = 0.0f;
    shared_data->kitchen_labor_cost = 0.0f;
    shared_data->bathroom_material_cost = 0.0f;
    shared_data->bathroom_labor_cost = 0.0f;
    shared_data->cleaning_material_cost = 0.0f;
    shared_data->cleaning_labor_cost = 0.0f;
    shared_data->roof_material_cost = 0.0f;
    shared_data->roof_labor_cost = 0.0f;

    // Mutex'i paylaşılan bellekte başlat
    pthread_mutexattr_t mutex_attr; //Mutex özelliklerini tanımlamak için bir mutex öznitelik nesnesi oluştur
    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED); // Mutex'in süreçler arasında paylaşılması sağlanır
                                                                       //PTHREAD_PROCESS_SHARED ayarı, mutex'in yalnızca aynı süreç içindeki iş parçacıkları
                                                                       //tarafından değil, farklı süreçler tarafından da kullanılabileceğini belirtir.
    pthread_mutex_init(&shared_data->cost_mutex, &mutex_attr);         //Paylaşılan bellekteki mutex'i başlat:
    pthread_mutexattr_destroy(&mutex_attr);                            // Mutex öznitelik nesnesini yok et:

    // Mevcut semaforları temizle
    char sem_name[32];
    for (int i = 0; i < TOTAL_FLOORS; i++) {
        snprintf(sem_name, sizeof(sem_name), "/floor_sem_%d", i);
        sem_unlink(sem_name); // Eski semaforları kaldır
    }

    // Adlandırılmış semaforları başlat
    for (int i = 0; i < TOTAL_FLOORS; i++) {
        snprintf(sem_name, sizeof(sem_name), "/floor_sem_%d", i);
        floor_semaphore[i] = sem_open(sem_name, O_CREAT, 0644, 0); // Yeni semafor oluştur
        if (floor_semaphore[i] == SEM_FAILED) {
            perror("sem_open failed");
            floor_semaphore[i] = sem_open(sem_name, 0); //Hata durumunda tekrar dene
            if (floor_semaphore[i] == SEM_FAILED) {
                perror("sem_open retry failed");
                exit(EXIT_FAILURE);
            }
        }
    }

    // Diğer senkronizasyon nesnelerini başlat
    sem_init(&elevator_sem, 0, 1); // Asansör semaforu (tek kullanıcı)
    sem_init(&stairs_sem, 0, 2);   // Merdiven semaforu (2 kullanıcı)

    // Simülasyon başlangıç mesajı
    printf(COLOR_GREEN "\n10 KATLI APARTMAN İNŞAAT SIMÜLASYONU BAŞLIYOR\n" COLOR_RESET);
    printf("Toplam süre: %d gün\n", TOTAL_DAYS);
    printf("====================================\n");

    build_foundation(); // Temel atma işlemini gerçekleştir

    // Her kat için kaba inşaat işlemi
    for (int floor = 0; floor < TOTAL_FLOORS; floor++) {
        if (floor > 0) {
            printf("Ana süreç: Kat %d için semafor bekleniyor\n", floor+1);
            if (sem_wait(floor_semaphore[floor]) == -1) { // Bir önceki katın tamamlanmasını bekle
                perror("sem_wait failed");
                exit(EXIT_FAILURE);
            }
        }

        print_day_info(); // Gün bilgisini yazdır

        pid_t pid = fork();             // Yeni süreç oluştur
        if (pid == 0) {                 // Çocuk süreç
            floor_construction(floor);  // Kat inşaatını gerçekleştir
            exit(0);                    // Çocuk süreci sonlandır
        } else if (pid > 0) {           // Ana süreç
            waitpid(pid, NULL, 0);      // Çocuk sürecin tamamlanmasını bekle
        } else {
            perror("fork hatası");      // Hata kontrolü
            exit(EXIT_FAILURE);
        }

        // Son katın kaba inşaatından sonra çatı iskeleti yapımı
        if (floor == TOTAL_FLOORS - 1) {
            print_day_info();
            roof_skeleton(); // Çatı iskeletini oluştur
        }
    }

    // Tüm dairelerin iç dizayn işlemleri
    printf(COLOR_CYAN "\n=== TÜM DAİRELERİN İÇ DİZAYN İŞLEMLERİ BAŞLADI ===\n" COLOR_RESET);
    for (int floor = 0; floor < TOTAL_FLOORS; floor++) {
        print_day_info();
        printf(COLOR_CYAN "=== KAT %d İÇ DİZAYN İŞLEMLERİ BAŞLADI ===\n" COLOR_RESET, floor+1);

        pthread_t threads[APARTMENTS_PER_FLOOR]; // Her daire için thread dizisi
        for (int i = 0; i < APARTMENTS_PER_FLOOR; i++) {
            int* args = malloc(2 * sizeof(int)); // Kat ve daire numarası için dinamik bellek tahsisi
            args[0] = floor;
            args[1] = i;
            pthread_create(&threads[i], NULL, apartment_construction, args); // Thread oluştur
        }

        for (int i = 0; i < APARTMENTS_PER_FLOOR; i++) {
            pthread_join(threads[i], NULL); // Thread'lerin tamamlanmasını bekle
        }

        printf(COLOR_CYAN "=== KAT %d İÇ DİZAYN İŞLEMLERİ TAMAMLANDI ===\n\n" COLOR_RESET, floor+1);
    }
    printf(COLOR_CYAN "=== TÜM DAİRELERİN İÇ DİZAYN İŞLEMLERİ TAMAMLANDI ===\n\n" COLOR_RESET);

    // Çatı katı ve ortak alanlar
    print_day_info();
    printf(COLOR_RED "\n=== ÇATI KATI İNŞAATI BAŞLADI ===\n" COLOR_RESET);
    printf("Çatı yapımı devam ediyor...\n");
    random_delay(2.0, 3.0); // Rastgele gecikme
    float roof_material_cost1 = 30000.0f; // Malzeme maliyeti
    float roof_labor_cost1 = roof_material_cost1 * 0.4; // İşçilik maliyeti
    add_cost(roof_material_cost1, roof_labor_cost1, "roof"); // Maliyeti ekle

    printf("Asansör sistemleri kuruluyor...\n");
    random_delay(1.0, 2.0); // Rastgele gecikme
    float roof_material_cost2 = 30000.0f; // Malzeme maliyeti
    float roof_labor_cost2 = roof_material_cost2 * 0.4; // İşçilik maliyeti
    add_cost(roof_material_cost2, roof_labor_cost2, "roof"); // Maliyeti ekle

    printf("Ortak alanların dekorasyonu yapılıyor...\n");
    random_delay(1.5, 2.5); // Rastgele gecikme
    float roof_material_cost3 = 20000.0f; // Malzeme maliyeti
    float roof_labor_cost3 = roof_material_cost3 * 0.4; // İşçilik maliyeti
    add_cost(roof_material_cost3, roof_labor_cost3, "roof"); // Maliyeti ekle

    printf(COLOR_RED "=== ÇATI KATI TAMAMLANDI ===\n\n" COLOR_RESET);

    // Simülasyon sonu özeti
    printf(COLOR_GREEN "\n=== İNŞAAT TAMAMLANDI ===\n" COLOR_RESET);
    printf("Toplam süre: %d gün\n", construction_day-1); // Toplam gün sayısı
    printf("Toplam maliyet: %.2f TL\n", shared_data->total_cost); // Toplam maliyet
    printf("Kat başına ortalama maliyet: %.2f TL\n", shared_data->total_cost/TOTAL_FLOORS); // Kat başına maliyet
    printf("Daire başına ortalama maliyet: %.2f TL\n",
           shared_data->total_cost/(TOTAL_FLOORS*APARTMENTS_PER_FLOOR)); // Daire başına maliyet

    // Maliyet özetini yazdır
    print_cost_summary();

    // Kaynakları temizle
    pthread_mutex_destroy(&shared_data->cost_mutex); // Mutex'i yok et
    munmap(shared_data, sizeof(struct SharedData)); // Paylaşılan belleği serbest bırak
    shm_unlink("/construction_shm"); // Paylaşılan bellek dosyasını kaldır

    sem_destroy(&elevator_sem); // Asansör semaforunu yok et
    sem_destroy(&stairs_sem);  // Merdiven semaforunu yok et

    // Kat semaforlarını temizle
    for (int i = 0; i < TOTAL_FLOORS; i++) {
        snprintf(sem_name, sizeof(sem_name), "/floor_sem_%d", i);
        sem_close(floor_semaphore[i]); // Semaforu kapat
        sem_unlink(sem_name); // Semaforu kaldır
    }

    return 0; // Programı başarıyla sonlandır
}
