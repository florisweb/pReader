
wallThickness = 1.5;
height = 5;

femaleConnWidth = 2.6;
femaleConnHeight = 3;
femaleConnCableSize = 1.25;
femaleConnLength = 14.5;

rightSideEnsurerWidth = 2;

depth = femaleConnLength + wallThickness;

femalePedalConnector();

translate([20, 0, 0])
malePedalConnector(); 


module femalePedalConnector() {
    width = femaleConnWidth * 3 + wallThickness * 2 + rightSideEnsurerWidth;
    difference() {
       difference() {
            cube([width, depth, height]);

            translate([0, 0, (height - femaleConnHeight)/2]) {

                translate([wallThickness, 0, 0])
                cube([femaleConnWidth * 2, femaleConnLength, femaleConnHeight]);
                translate([wallThickness + femaleConnWidth * 2 + rightSideEnsurerWidth, 0, 0])
                
                cube([femaleConnWidth, femaleConnLength, femaleConnHeight]);
                
                translate([(femaleConnWidth - femaleConnCableSize)/2, femaleConnLength, 0]) {
                    translate([wallThickness, 0, 0])
                    cube([femaleConnCableSize, wallThickness, 10]);
                    translate([wallThickness + femaleConnWidth, 0, 0])                
                    cube([femaleConnCableSize, wallThickness, 10]);
                    
                    translate([wallThickness + femaleConnWidth * 2 + rightSideEnsurerWidth, 0, 0])
                    cube([femaleConnCableSize, wallThickness, 10]);
                }
            }
        }
        translate([0, wallThickness, (height + femaleConnHeight)/2])
        cube([width, depth, height]);
    }
}


module malePedalConnector() {
    padding = 1;
    connMoveDepth = 5;
    difference() {
        difference() {
            cube([
                femaleConnWidth * 3 + rightSideEnsurerWidth + padding * 2, 
                depth, 
                femaleConnHeight + padding * 2
            ]);
            translate([0, 0, padding]) {
                translate([padding, 0, 0])
                cube([femaleConnWidth * 2, femaleConnLength, femaleConnHeight]);
                translate([padding + femaleConnWidth * 2 + rightSideEnsurerWidth, 0, 0])
                
                cube([femaleConnWidth, femaleConnLength, femaleConnHeight]);
                
                translate([(femaleConnWidth - femaleConnCableSize)/2, femaleConnLength, 0]) {
                    translate([padding, 0, 0])
                    cube([femaleConnCableSize, wallThickness, 10]);
                    translate([padding + femaleConnWidth, 0, 0])                
                    cube([femaleConnCableSize, wallThickness, 10]);
                    
                    translate([padding + femaleConnWidth * 2 + rightSideEnsurerWidth, 0, 0])
                    cube([femaleConnCableSize, wallThickness, 10]);
                }
            }
        }
        translate([padding, depth - connMoveDepth, femaleConnHeight + padding])
        cube([
            femaleConnWidth * 3 + rightSideEnsurerWidth,
            connMoveDepth, 
            femaleConnHeight + padding * 2
        ]);
    }
}