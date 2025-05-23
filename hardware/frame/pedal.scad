include <lib.scad>
$fn = 50;


screwRadiusM3 = 1.45; // The M3x10mm screws



// PEDAL
pedalLength = 130; // 50
pedalWidth = 10;
pedalStickHeight = 7;

pedalConnHeight = 10;
pedalConnLength = 10;
pedalConnHoleRad = screwRadiusM3 + .2;


// PEDALHOLDER


pedalFeetLength = 50;
pedalFeetHeight = 3;
pedalFeetWidth = 40;


pedalHolderWallThickness = 3;
pedalMargin = .1;
pedalHolderConnHeight = 27;
pedalHolderWidth = pedalWidth + 2 * (pedalHolderWallThickness + pedalMargin);
pedalHolderLength = 17;
pedalHolderHeight = 32;



pedalOffset = 80;
footPadding = 10;
footDepth = pedalLength + footPadding * 2;
footGroundHeight = 3;
footWidth = pedalHolderWidth + footPadding * 2 + pedalOffset;

pedalSpringOffset = 50;



metalWireHeight = 5;
metalWireOffset = 2;
metalWireRadius = .5;


// PEDAL SPRING
springDepth = 10;
springThickness = 2;

//
////translate([-pedalOffset/2, 
////    -pedalConnLength / 2 + pedalHolderWallThickness + pedalConnLength * 2/3, pedalHolderConnHeight + footGroundHeight + -(pedalStickHeight - pedalConnHeight / 2)
////]) {
////    rotate([-5, 0, 0]) {
////        pedal();
////        translate([pedalOffset, 0, 0])
////        backPedal();
////    }
//}
//
//translate([-footWidth/2, -footPadding, 0])
//pedalHolder();

//translate([0, pedalSpringOffset, footGroundHeight-springThickness])
//pedalSpring();

backPedal();


module pedalSpring() {
    translate([pedalWidth / 2, springDepth, 0])
    rotate([90, 0, -90])
    union() {
        translate([0, pedalHolderConnHeight, 0])
        rotate([0, 0, 180])
        spring(springDepth, pedalHolderConnHeight - springThickness, pedalWidth, 2, springThickness);

        translate([-springDepth * 2, 0, 0])
        cube([springDepth * 3, springThickness, pedalWidth]);
    }
}
    


module pedalHolder() {
    difference() {
        boxWithRoundedCorners(footWidth, footDepth, footGroundHeight, 3);
        singlePedal();
        translate([pedalOffset, 0, 0])
        singlePedal();
    }
    difference() {
        union() {
            singlePedal();
            translate([pedalOffset, 0, 0])
            singlePedal();
        }
        boxWithRoundedCorners(footWidth, footDepth, footGroundHeight, 3);
    }
    
    
    // CABLE GUIDER
    cableGuiderHoleRad = 2;
    guiderSize = 7;

    translate([footWidth/2, 3, 0])
    rotate([90, 0, 0])
    difference() 
    {
        translate([-guiderSize/2, 0, 0])
        boxWithRoundedCorners(guiderSize, 8, 3, 2);
        translate([0, footGroundHeight+cableGuiderHoleRad, 0])
        cylinder(3, r=cableGuiderHoleRad);
    }
}


module singlePedal() {
    translate([
        footPadding + pedalHolderWallThickness + pedalMargin, 
        footPadding + pedalSpringOffset, 
        footGroundHeight-springThickness
    ])        
    cube([pedalWidth, springDepth * 3, springThickness]);
        

    translate([footPadding, footPadding, footGroundHeight]) {
        difference() {
            cube([pedalHolderWidth, pedalLength, pedalHolderHeight]);
            translate([pedalHolderWallThickness, 0, 0])
            cube([pedalWidth + 2 * pedalMargin, pedalLength, pedalHolderHeight]);

            translate([0, pedalHolderWallThickness + pedalConnLength * 2/3, pedalHolderConnHeight])
            rotate([0, 90, 0])
            cylinder(pedalHolderWidth, r=screwRadiusM3);
            
            
            translate([0, metalWireOffset, metalWireHeight])
            rotate([0, 90, 0])
            cylinder(pedalHolderWidth, r=metalWireRadius);
            
            translate([0, metalWireOffset + 10, metalWireHeight])
            rotate([0, 90, 0])
            cylinder(pedalHolderWidth, r=metalWireRadius);
            
            angle = 16;
            translate([0, pedalHolderLength + pedalHolderHeight/tan(angle), 0])
            rotate([90 - angle, 0, 0])
            translate([0, 0, -50])
            cube([pedalHolderWidth, 200, 500]);
        }
    }
}
    
module backPedal() {
    mirror([1, 0, 0])
    pedal();
}
module pedal() {
    difference() 
    {
        union() {
            // Pedal Stick
            translate([pedalWidth/2, 0, 0])
            rotate([0, -90, 0])
            boxWithRoundedCorners(pedalStickHeight, pedalLength, pedalWidth, 3);
            
            // Pedal feet
            translate([-pedalFeetWidth / 2, pedalLength - pedalFeetLength, pedalStickHeight - pedalFeetHeight]) {
                boxWithRoundedCorners(pedalFeetWidth, pedalFeetLength, pedalFeetHeight, 3);
            }
            
            
            // Conn point
            translate([pedalWidth/2, 0, pedalStickHeight - pedalConnHeight])
            rotate([0, -90, 0])
            boxWithRoundedCorners(pedalConnHeight, pedalConnLength, pedalWidth, 3);
            
            wireBeamWidth = 5;
            wireBeamHeight = pedalHolderConnHeight - 3;
            translate([-pedalWidth/2 + wireBeamWidth/2, 0, -(wireBeamHeight)])
            difference() {
                cube([wireBeamWidth, 3, wireBeamHeight]);
                
                translate([wireBeamWidth/2, 0, 2])
                rotate([0, 90, 90])
                cylinder(pedalHolderWidth, r=metalWireRadius);
                translate([wireBeamWidth/2, 0, 10])
                rotate([0, 90, 90])
                cylinder(pedalHolderWidth, r=metalWireRadius);
            }
        }
        
        translate([-pedalWidth/2, pedalConnLength / 2, pedalStickHeight - pedalConnHeight / 2])
        rotate([0, 90, 0])
        cylinder(pedalWidth, r=pedalConnHoleRad);
        
        // Icon cutout
        width = pedalFeetWidth * .4;
        iconDepth = .5;
        translate([
            width * sqrt(4/3) / 3, 
            pedalLength - pedalFeetLength/2 + width * sqrt(4/3) / 2, 
            pedalStickHeight - iconDepth
        ])
        rotate([0, 0, 180])
        eqTriangle(width, iconDepth  *2);
       
   }
}



    
//module pedal() {
//    difference() 
//    {
//        union() {
//            // Pedal Stick
//            translate([pedalWidth/2, 0, 0])
//            rotate([0, -90, 0])
//            boxWithRoundedCorners(pedalStickHeight, pedalLength, pedalWidth, 3);
//            
//            // Pedal feet
//            translate([-pedalFeetWidth / 2, pedalLength - pedalFeetLength, pedalStickHeight - pedalFeetHeight]) {
//                boxWithRoundedCorners(pedalFeetWidth, pedalFeetLength, pedalFeetHeight, 3);
//            }
//            
//            
//            // Conn point
//            translate([pedalWidth/2, 0, pedalStickHeight - pedalConnHeight])
//            rotate([0, -90, 0])
//            boxWithRoundedCorners(pedalConnHeight, pedalConnLength, pedalWidth, pedalConnLength/2);
//        }
//        
//        translate([-pedalWidth/2, pedalConnLength / 2, pedalStickHeight - pedalConnHeight / 2])
//        rotate([0, 90, 0])
//        cylinder(pedalWidth, r=pedalConnHoleRad);
//        
//        // Icon cutout
//        width = pedalFeetWidth * .4;
//        iconDepth = .5;
//        translate([width * sqrt(4/3) / 3, pedalLength - pedalFeetLength/2 + width * sqrt(4/3) / 2, pedalStickHeight - iconDepth])
//        rotate([0, 0, 180])
//        eqTriangle(width, iconDepth  *2);
//        
//    }
//}