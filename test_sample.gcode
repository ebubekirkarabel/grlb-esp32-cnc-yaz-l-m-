; Test G-code dosyası
; Basit kare çizimi

G21 ; Metrik sistem
G90 ; Mutlak konumlandırma
G28 ; Home pozisyonuna git

; Başlangıç pozisyonu
G0 X0 Y0 Z5

; Kare çizimi
G1 X10 Y0 Z1 F1000
G1 X10 Y10 Z1 F1000
G1 X0 Y10 Z1 F1000
G1 X0 Y0 Z1 F1000

; Yukarı çık
G0 Z5

; İkinci kare
G1 X5 Y5 Z1 F1000
G1 X15 Y5 Z1 F1000
G1 X15 Y15 Z1 F1000
G1 X5 Y15 Z1 F1000
G1 X5 Y5 Z1 F1000

; Bitiş
G0 Z10
G0 X0 Y0
M2 ; Program sonu 