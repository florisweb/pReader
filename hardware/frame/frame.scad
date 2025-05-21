
$fn=25;

screwRadius = 3.7/2;
screwRadiusM3 = 1.45; // The M3x10mm screws
boltSideMargin = 4; // Distance of centre of a bolt to the side of the frame

wallThickness = 1.5;
topThickness = 0.8;
bottomThickness = 1;



// SCREEN
innerScreenLeft = 3;
innerScreenTop = 4;
innerScreenWidth = 198.16;
innerScreenDepth = 279.52;
screenHeight = 1.5;
screenWidth = 212.26;

screenMargin = .5;
extraSideWidth = 23;

width = screenWidth + extraSideWidth + wallThickness * 2 + screenMargin * 2;
depth = 286.32 + wallThickness * 2 + screenMargin * 2;
height = 10 + topThickness;





// ======== FRONT ========
// ESP
ESPDepth = 52;
ESPPortWidth = 9;
ESPPortHeight = 3.5;
ESPPortLeftOffset = 10;


// Buttons
buttonSetInterval = 20;
buttonSetTopOffset = 80;

buttonWidth = 3.5;
buttonHeight = 3;
buttonDepth = 10;
buttonClampPadding = 1;
buttonClampWidth = .5;
buttonHoleMargin = .2;

// True Button
TButtonWidth = 3.2;
TButtonHeight = 6;
TButtonDepth = 6;
TButtonPressableWidth = 1.5;


// Pedal Connector
pedalConnectorBottomOffset = 20;
femaleConnWidth = 2.6;
femaleConnHeight = 3;
femaleConnCableSize = 1.25;
femaleConnLength = 14.5;

rightSideEnsurerWidth = 2;
femalePedalConnectorWidth = femaleConnWidth * 3 + wallThickness * 2 + rightSideEnsurerWidth;


// ======== BACK ========
backConnWidth = 20;
backConnHeight = 7;

   
    



mirror([1, 0, 0])
difference() 
{
    union() {
//        frontHalf();
//        translate([0, 0, height])
        backHalf();
    }
    
    translate([-10, depth / 2, -50]) cube([1000, depth / 2 + 10, 1000]);
//    translate([-10, -10, -50]) cube([1000, depth / 2 + 10, 1000]);
}



module backHalf() {
    difference() {
        cube([width, depth, bottomThickness]);

        
        // LOGO
        translate([
            width / 2 + 26.5, depth / 2 - 25, bottomThickness - 0.15]) {
            linear_extrude(.15)
            scale([.14, .14, 1])
            rotate([0, 180, 0])
            import("/Library/WebServer/Documents/git/AutoHomeProjects/pReader/hardware/frame/readerLogo.svg");
        }
        
        // TOP
        translate([width - 23 - boltSideMargin - extraSideWidth, depth - boltSideMargin, 0])
        cylinder(r=screwRadiusM3, h=20);

        translate([boltSideMargin, depth / 2 + boltSideMargin, 0])
        cylinder(r=screwRadiusM3, h=20);

        translate([width - boltSideMargin, depth / 2 + boltSideMargin, 0])
        cylinder(r=screwRadiusM3, h=20);
        
        translate([width - boltSideMargin, depth - 57.5, 0])
        cylinder(r=screwRadiusM3, h=20);
        
        translate([boltSideMargin, depth - boltSideMargin, 0])
        cylinder(r=screwRadiusM3, h=20);
        
        translate([width / 2, depth - boltSideMargin, 0])
        cylinder(r=screwRadiusM3, h=20);
        
        
        
        // BOTTOM
        translate([boltSideMargin, boltSideMargin, 0])
        cylinder(r=screwRadiusM3, h=20);
        
        translate([width - boltSideMargin, boltSideMargin, 0])
        cylinder(r=screwRadiusM3, h=20);
        
        translate([width / 2, boltSideMargin, 0])
        cylinder(r=screwRadiusM3, h=20);
        
        translate([boltSideMargin, depth / 2 - boltSideMargin, 0])
        cylinder(r=screwRadiusM3, h=20);
        
        translate([width - boltSideMargin, depth / 2 - boltSideMargin, 0])
        cylinder(r=screwRadiusM3, h=20);

    }
    
    
    
    translate([screenMargin+wallThickness + 7, 0, 0]) {
       backConnector();
    }
    
    translate([width/2 - backConnWidth/2 - 40, 0, 0]) {
       backConnector();
    }
    
    translate([width - (screenMargin+wallThickness + 40) - backConnWidth, 0, 0]) {
       backConnector();
    }
    
    // Blocks to keep the screen pinned down
    // TOP
    translate([0, depth - 10 - (screenMargin+wallThickness), -(height - screenHeight)])
    {
        translate([10, 0, 0])
        cube([10, 10, height - screenHeight]);
        translate([width/2 - 17, 0, 0])
        cube([10, 10, height - screenHeight]);
        translate([width - 10 - extraSideWidth - 12, 0, 0])
        cube([10, 10, height - screenHeight]);
    }
    
    translate([0, screenMargin+wallThickness, -(height - screenHeight)])
    {
        translate([10, 0, 0])
        cube([10, 10, height - screenHeight]);
        translate([width/2 + 6, 0, 0])
        cube([10, 10, height - screenHeight]);
        translate([width - 10 - extraSideWidth - 12, 0, 0])
        cube([10, 10, height - screenHeight]);
    }
    
    // Ribs for stability
    translate([0, (screenMargin+wallThickness), -2]) {
        translate([50, 0, 0])
        cube([5, depth - (screenMargin+wallThickness) * 2, 2]);
        translate([150, 0, 0])
        cube([5, depth - (screenMargin+wallThickness) * 2, 2]);
        translate([196.5, 0, 0])
        cube([5, depth - (screenMargin+wallThickness) * 2, 2]);
    }
}

module backConnector() {
    translate([0, depth/2 - backConnWidth/2, -backConnHeight]) {
        difference() {
            cube([20, backConnWidth, backConnHeight]);

            translate([5, 20, backConnHeight/2 - .5])
            rotate([90, 0, 0])
            cylinder(r=screwRadiusM3, h=50);
            
            translate([15, 20, backConnHeight/2 - .5])
            rotate([90, 0, 0])
            cylinder(r=screwRadiusM3, h=50);
        }
    }
}
    


module frontHalf() {
    difference() {
        mainFrameFrontHalf();
        translate([width - wallThickness - ESPPortWidth - ESPPortLeftOffset, depth - wallThickness, topThickness + 0.5])
        cube([ESPPortWidth, wallThickness, ESPPortHeight]);
    }
}

module mainFrameFrontHalf() {
    difference()
    {
        cube([width, depth, height]);
        translate([
            innerScreenLeft + wallThickness + screenMargin, -(innerScreenTop + wallThickness - (depth - innerScreenDepth)) + screenMargin, 0])
        cube([innerScreenWidth, innerScreenDepth, 100]);
        
        
        translate([wallThickness, wallThickness, topThickness])
        cube([width - wallThickness * 2, depth - wallThickness * 2, height]);
            
        // Cut out a hole in the wall for the pedalConnector
        translate([width, pedalConnectorBottomOffset, topThickness])
        rotate([0, 0, 90])
        cube([femalePedalConnectorWidth, wallThickness, height]);
        
            
        translate([
             width, 
            -buttonHoleMargin + depth - buttonSetTopOffset, 
            -buttonHoleMargin + (height - topThickness) / 2 + topThickness - (buttonHeight + 2          *buttonHoleMargin) / 2])
        { 
            
            rotate([0, -90, 0])
            boxWithRoundedCorners(
                buttonHeight + buttonHoleMargin * 2, 
                buttonDepth + buttonHoleMargin * 2, 
                wallThickness * 2, 
                .5
            );
            
            translate([0, -buttonSetInterval, 0])
            rotate([0, -90, 0])
            boxWithRoundedCorners(
                buttonHeight + buttonHoleMargin * 2, 
                buttonDepth + buttonHoleMargin * 2, 
                wallThickness * 2, 
                .5
            );
            
            translate([0, -buttonSetInterval * 2, 0])
            rotate([0, -90, 0])
            boxWithRoundedCorners(
                buttonHeight + buttonHoleMargin * 2, 
                buttonDepth + buttonHoleMargin * 2, 
                wallThickness * 2, 
                .5
            );
            
        }
          
        // Insert the labels/icons
        translate([
                width - (buttonClampWidth + TButtonPressableWidth + wallThickness) - 7 - extraSideWidth / 2, 
                -buttonHoleMargin + depth - buttonSetTopOffset, 
                0])
        {
            eqTriangle(10, .15);     

            translate([0, 3 - buttonSetInterval, 0])
            linear_extrude(.15)
            text("OK", size=5);
            
            translate([10, 11 - buttonSetInterval * 2, 0])
            rotate([0, 0, 180])
            eqTriangle(10, .15);
        }
        
        
        translate([
            width - 20.5 - extraSideWidth / 2, 
            -buttonHoleMargin + depth - 45, 0]) {
            linear_extrude(.15)
            scale([.07, .07, 1])
            import("/Library/WebServer/Documents/git/AutoHomeProjects/pReader/hardware/frame/readerLogo.svg");
        }

    }
    
    
    // BUTTONS
    translate([
            width - (buttonClampWidth + TButtonPressableWidth + wallThickness), 
            -buttonHoleMargin + depth - buttonSetTopOffset + 8.5, 
            topThickness])
    {
        rotate([0, 0, 180])
        buttonHolder();
        
        translate([0, -buttonSetInterval, 0])
        rotate([0, 0, 180])
        buttonHolder();
        
        translate([0, -buttonSetInterval * 2, 0])
        rotate([0, 0, 180])
        buttonHolder();
    }
    
    
    
    // PEDAL-CONNECTOR
    translate([width, pedalConnectorBottomOffset, 0])
    rotate([0, 0, 90])
    femalePedalConnector();
    
    
    
    // DISPLAY STOPPERS: keep the display in place
    stopperWidth = width - screenWidth - wallThickness * 2 - screenMargin * 2;
    translate([width, 0, 0]) 
    difference() {
        translate([-stopperWidth - wallThickness, wallThickness, 0]) 
        cube([stopperWidth, 8, height]);
        translate([-boltSideMargin, boltSideMargin, 0])
        cylinder(r=screwRadiusM3, h=30);
    }
    
    translate([width, depth - wallThickness - 8 - ESPDepth, 0]) 
    difference() {
        translate([-stopperWidth - wallThickness, 0, 0]) 
        cube([stopperWidth, 8, height]);
        translate([-boltSideMargin, 4, 0])
        cylinder(r=screwRadiusM3, h=30);
    }
    
    verticalConnectorSize = 20;
    translate([
        width, 
        depth / 2 - verticalConnectorSize / 2, 
    0]) {
        difference() 
        {
            translate([- verticalConnectorSize - wallThickness, 0, 0]) {
                difference() {
                    cube([verticalConnectorSize, verticalConnectorSize, height]);
                    
                    // Vertical bolt hole
                    translate([20 - 13, 30, height / 2])
                    rotate([90, 0, 0])
                    cylinder(r=screwRadius, h=50);
                }
            }
            
            translate([-boltSideMargin, verticalConnectorSize/2 + boltSideMargin, 0])
            cylinder(r=screwRadiusM3, h=30);
            
            translate([-boltSideMargin, verticalConnectorSize/2 - boltSideMargin, 0])
            cylinder(r=screwRadiusM3, h=30);
            
           
        }
    }
    
    
    // BACK-CONNECTORS
    // BOTTOM
    translate([boltSideMargin, boltSideMargin, topThickness+screenHeight - 1])
    rotate([0, 0, -90])
    triConnectionPoint();
    
    translate([boltSideMargin, depth / 2 - boltSideMargin, topThickness+screenHeight - 1])
    rotate([0, 0, -90])
    triConnectionPoint();
    
    translate([width / 2, boltSideMargin, topThickness+screenHeight - 1])
    triConnectionPoint(); 
    
    // TOP
    translate([boltSideMargin, depth / 2 + boltSideMargin, topThickness+screenHeight - 1])
    rotate([0, 0, -90])
    triConnectionPoint();
    
    translate([boltSideMargin, depth - boltSideMargin, topThickness+screenHeight - 1])
    rotate([0, 0, -90])
    triConnectionPoint();
    
    translate([width - extraSideWidth * 2 - boltSideMargin, depth - boltSideMargin, topThickness+screenHeight - 1])
    rotate([0, 0, 180])
    triConnectionPoint();
    
    translate([width / 2, depth - boltSideMargin, topThickness+screenHeight - 1])
    rotate([0, 0, 180])
    triConnectionPoint(); 
}




module femalePedalConnector() {
    depth = femaleConnLength + wallThickness;
    difference() {
       difference() {
            cube([femalePedalConnectorWidth, depth, height]);

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
        cube([femalePedalConnectorWidth, depth, height]);
    }
}












module triConnectionPoint(width = boltSideMargin * 2, depth = boltSideMargin * 2) {
    boxHeight = 5;
    prismHeight = height - topThickness - screenHeight - boxHeight + 1;
    translate([width/2, depth/2, 0])
    difference() 
    {
        union() 
        {
            rotate([0, -90, 0])
            translate([0, -depth, 0])
            prism(prismHeight, depth, width);
            translate([-width, -depth, prismHeight])
            cube([width, depth, boxHeight]);
        }
        
        translate([-width/2, -depth/2, -3])
        cylinder(r=screwRadiusM3, h=30);
    }
}
    
    

module prism(width, depth, height) {
    
    triangle_points = [
        [0, 0],  // Point A
        [width, 0], // Point B
        [width, depth]  // Point C
    ];

    linear_extrude(height = height) {
        polygon(points = triangle_points);
    }
}





module buttonHolder() {
    thickness = 4;
    trueWidth = TButtonWidth - 1;
    
    cube([TButtonWidth, TButtonDepth, (height - topThickness) / 2 - TButtonHeight / 2]);
    
    translate([TButtonWidth, 0, 0])
    cube([thickness, TButtonDepth, height - topThickness]);
    
    translate([0, TButtonDepth, 0])
    cube([trueWidth, thickness, height - topThickness]);
    
    translate([0, -thickness, 0])
    cube([trueWidth, thickness, height - topThickness]);
}


module button(width = buttonWidth, height = buttonHeight, depth = buttonDepth, clampPadding = buttonClampPadding, clampWidth = buttonClampWidth) {
    
    translate([width, 0, 0])
    rotate([0, -90, 0])
    boxWithRoundedCorners(height, depth, width - clampWidth, .5);
    
    
    translate([width - clampWidth, -clampPadding, -clampPadding])
    cube([
        clampWidth, 
        depth + clampPadding * 2, 
        height + clampPadding * 2
    ]);
}

module boxWithRoundedCorners(width, depth, height, cornerRadius) {
    innerWidth = width - cornerRadius * 2;
    innerDepth = depth - cornerRadius * 2;
    
    translate([cornerRadius, 0, 0])
    cube([innerWidth, depth, height]);
    translate([0, cornerRadius, 0])
    cube([width, innerDepth, height]);
    
    translate([cornerRadius, cornerRadius, 0])
    cylinder(h=height, r=cornerRadius);
    translate([width - cornerRadius, cornerRadius, 0])
    cylinder(h=height, r=cornerRadius);
    translate([width - cornerRadius, depth - cornerRadius, 0])
    cylinder(h=height, r=cornerRadius);
    translate([cornerRadius, depth - cornerRadius, 0])
    cylinder(h=height, r=cornerRadius);
}



module eqTriangle(width, height) {    
    triangle_points = [
        [0, 0],  // Point A
        [width, width * sqrt(4/3) / 2], // Point B
        [0, width * sqrt(4/3)]  // Point C
    ];

    linear_extrude(height = height) {
        polygon(points = triangle_points);
    }
}

