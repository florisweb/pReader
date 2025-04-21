
$fn=25;

screwRadiusM5 = 3.7/2; // ?
screwRadiusM3 = 1.55;
screwRadiusM3_smooth = 1.6; // 1.65 for sure

difference() {
    translate([-3, -4, 0])
    cube([6, 37, 5]);
    
    screwRadiusM3 = 2.7/2;
  
    cylinder(r=screwRadiusM3 + .3, 5);
    translate([0, 5, 0])
    cylinder(r=screwRadiusM3 + .2, 5);
    translate([0, 5*2, 0])
    cylinder(r=screwRadiusM3 + .1, 5);
    translate([0, 5*3, 0])
    cylinder(r=screwRadiusM3, 5);
    translate([0, 5*4, 0])
    cylinder(r=screwRadiusM3 - .1, 5);
    translate([0, 5*5, 0])
    cylinder(r=screwRadiusM3 - .2, 5);
    translate([0, 5*6, 0])
    cylinder(r=screwRadiusM3 - .3, 5);
}