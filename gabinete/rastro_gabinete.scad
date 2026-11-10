// RASTRO — gabinete para impressão 3D (OpenSCAD, paramétrico)
// Peças: caixa (frente + paredes) e tampa traseira. Medidas em mm.
// Para exportar: escolha PECA, aperte F6 e depois F7 (Export STL).

PECA = "caixa";        // "caixa", "tampa" ou "montagem"

W = 130;  H = 150;  D = 68;   // medidas externas
T = 2.4;                      // espessura das paredes
GRAV = 0.8;                   // profundidade das gravações
EYE_Z = 100;  CAM_Z = 134;  LCD_Z = 54;
CX = W / 2;
$fn = 72;

// ---------- auxiliares (desenho no plano X-Z, extrudado em Y)
module noPlanoXZ(y0, esp) { translate([0, y0 + esp, 0]) rotate([90, 0, 0]) linear_extrude(esp) children(); }

module amendoa(w, h) {           // contorno do olho
    rr = (w*w/4 + h*h/4) / h;  off = rr - h/2;
    intersection() { translate([0, -off]) circle(rr, $fn = 240); translate([0, off]) circle(rr, $fn = 240); }
}
module anel(r, l) { difference() { circle(r); circle(r - l); } }

// ---------- caixa
module caixa() {
    difference() {
        union() {
            difference() {
                cube([W, D, H]);
                translate([T, T, T]) cube([W - 2*T, D, H - 2*T]);    // cavidade aberta atrás
            }
            // pilares dos parafusos da tampa
            for (px = [T + 4.6, W - T - 4.6], pz = [T + 4.6, H - T - 4.6])
                translate([px, T, pz]) rotate([-90, 0, 0]) cylinder(r = 4.6, h = D - T);
            // colunas do LCD 16x2 (furos 75 x 31), 7,2 mm = altura da moldura do display
            for (dx = [-37.5, 37.5], dz = [-15.5, 15.5])
                translate([CX + dx, T, LCD_Z + dz]) rotate([-90, 0, 0])
                    difference() { cylinder(r = 3, h = 7.2); cylinder(r = 1.1, h = 7.3); }
            // encaixe do módulo da câmera OV2640 (9 x 9 mm)
            noPlanoXZ(T, 4) translate([CX, CAM_Z]) difference() { square(13.4, center = true); square(9, center = true); }
            // anel guia da cúpula do PIR
            noPlanoXZ(T, 3) translate([CX, EYE_Z]) anel(15.5, 2);
        }
        // furos piloto M3 (parafuso auto-atarraxante)
        for (px = [T + 4.6, W - T - 4.6], pz = [T + 4.6, H - T - 4.6])
            translate([px, D - 16, pz]) rotate([-90, 0, 0]) cylinder(r = 1.3, h = 17);
        // furos passantes na frente
        noPlanoXZ(-1, T + 2) {
            translate([CX, EYE_Z]) circle(11.9);                    // cúpula do PIR = pupila
            translate([CX, CAM_Z]) circle(4.5);                     // lente da câmera
            translate([CX, LCD_Z]) square([66, 18], center = true); // janela do LCD
            translate([W - 22, 22]) {                               // grade do buzzer
                circle(1.25);
                for (a = [0:60:300]) translate([5.2*cos(a), 5.2*sin(a)]) circle(1.25);
            }
        }
        // gravações na face frontal
        noPlanoXZ(-1, GRAV + 1) {
            translate([CX, EYE_Z]) difference() { amendoa(104, 50); amendoa(99, 45); }
            translate([CX, EYE_Z]) anel(19, 1.4);
            translate([CX, CAM_Z]) anel(8, 1.2);
            translate([CX + 13, CAM_Z]) circle(2);
            translate([CX, LCD_Z]) difference() { square([76, 28], center = true); square([73.6, 25.6], center = true); }
            translate([CX - 6, 22]) text("RASTRO", size = 8, font = "Liberation Sans:style=Bold",
                                                       halign = "center", valign = "center", spacing = 1.12);
        }
        // passagem do cabo USB (parede de baixo)
        translate([W - 30, D - 12, -1]) cylinder(r = 5.5, h = T + 2);
        // ventilação no topo
        for (i = [0:4]) translate([W/2 - 24 + 12*i - 1.2, D - 40, H - T - 1]) cube([2.4, 28, T + 2]);
    }
}

// ---------- tampa traseira
module tampa() {
    difference() {
        cube([W, T, H]);
        noPlanoXZ(-1, T + 2) {
            for (px = [T + 4.6, W - T - 4.6], pz = [T + 4.6, H - T - 4.6]) translate([px, pz]) circle(1.75);
            for (kx = [W/2 - 35, W/2 + 35]) translate([kx, H - 30]) { circle(4.2); translate([-2.2, 0]) square([4.4, 12]); }
            for (i = [0:5]) translate([W/2, 30 + 6*i]) square([50, 2.4], center = true);
        }
        translate([0, T - GRAV, 0]) noPlanoXZ(0, GRAV + 1)
            translate([W/2, 80]) mirror([1, 0]) text("RASTRO  MOSTRA PIC", size = 5,
                font = "Liberation Sans:style=Bold", halign = "center", valign = "center");
    }
}

if (PECA == "caixa")        rotate([90, 0, 0]) caixa();   // frente apoiada na mesa
else if (PECA == "tampa")   rotate([90, 0, 0]) tampa();   // face gravada para cima
else { caixa(); translate([0, D + 0.5, 0]) color("gray") tampa(); }
