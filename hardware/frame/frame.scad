
$fn=25;

screwRadius = 3.7/2;
screwRadiusM3 = 1.45; // The M3x10mm screws
wallThickness = 1.5;
topThickness = 0.7;
bottomThickness = 1;

innerScreenLeft = 3;
innerScreenTop = 4;
innerScreenWidth = 198.16;
innerScreenDepth = 279.52;
screenHeight = 1.5;

ESPDepth = 52;
ESPPortWidth = 9;
ESPPortHeight = 3.5;
ESPPortLeftOffset = 10;

screenWidth = 212.26;
extraSideWidth = 23;
screenMargin = .5;
width = screenWidth + extraSideWidth + wallThickness * 2 + screenMargin * 2;
depth = 286.32 + wallThickness * 2 + screenMargin * 2;
height = 10 + topThickness;



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




backConnWidth = 20;
backConnHeight = 7;

   
    


mirror([1, 0, 0])
difference() 
{
    union() {
//        frontHalf();
        translate([0, 0, height])
        backHalf();
    }
    
    translate([-10, depth / 2, -50]) cube([1000, depth / 2 + 10, 1000]);
//    translate([-10, -10, -50]) cube([1000, depth / 2 + 10, 1000]);
}



module backHalf() {
    difference() {
        cube([width, depth, bottomThickness]);
        translate([5, 4, 0])
        cylinder(r=screwRadiusM3, h=20);
        
        translate([width - 6.5, 5.5, 0])
        cylinder(r=screwRadiusM3, h=20);
        
        translate([width / 2, 5.5, 0])
        cylinder(r=screwRadiusM3, h=20);
        
        translate([4, depth / 2 - 5, 0])
        cylinder(r=screwRadiusM3, h=20);
        
        translate([width - 6.5, depth / 2 - 5, 0])
        cylinder(r=screwRadiusM3, h=20);
        
        
        translate([4, depth / 2 + 5, 0])
        cylinder(r=screwRadiusM3, h=20);
        
        translate([width - 6.5, depth / 2 + 5, 0])
        cylinder(r=screwRadiusM3, h=20);
        
        
        translate([width - 28 - extraSideWidth, depth - 4, 0])
        cylinder(r=screwRadiusM3, h=20);
        
        translate([width - 6.5, depth - 57.5, 0])
        cylinder(r=screwRadiusM3, h=20);
        
        translate([4, depth - 5, 0])
        cylinder(r=screwRadiusM3, h=20);
         
        
        translate([
            width / 2 + 26.5, depth / 2 - 25, bottomThickness - 0.15]) {
            linear_extrude(.15)
            scale([.14, .14, 1])
            rotate([0, 180, 0])
            import("/Library/WebServer/Documents/git/AutoHomeProjects/pReader/hardware/frame/readerLogo.svg");
        }
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
    translate([0, depth - 10 - (screenMargin+wallThickness), -(height - screenHeight)])
    {
        translate([10, 0, 0])
        cube([10, 10, height - screenHeight]);
        translate([width/2 - 5, 0, 0])
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
    

    stopperWidth = width - screenWidth - wallThickness * 2 - screenMargin * 2;
    translate([width - stopperWidth - wallThickness, wallThickness, 0]) 
    difference() {
        cube([stopperWidth, 8, height]);
        translate([20 - 2, 4, 0])
        cylinder(r=screwRadiusM3, h=30);
    }
    
    translate([width - stopperWidth - wallThickness, depth - wallThickness - 8 - ESPDepth, 0]) 
    difference() {
        cube([stopperWidth, 8, height]);
        translate([20 - 2, 4, 0])
        cylinder(r=screwRadiusM3, h=30);
    }
    
    
    translate([
        width - 20 - wallThickness, 
        depth / 2 - 20 / 2, 
    0]) {
        difference() 
        {
            cube([20, 20, height]);
            translate([20 - 5, 5, 0])
            cylinder(r=screwRadiusM3, h=30);
            
            translate([20 - 5, 5 + 10, 0])
            cylinder(r=screwRadiusM3, h=30);
            
            translate([20 - 13, 30, height / 2])
            rotate([90, 0, 0])
            cylinder(r=screwRadius, h=50);
        }
    }
    
    
        
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
    
    
    translate([10, 8, topThickness+screenHeight - 1])
    triConnectionPoint();
    
    translate([8, depth / 2 - 10, topThickness+screenHeight - 1])
    rotate([0, 0, -90])
    triConnectionPoint();
    translate([8, depth / 2, topThickness+screenHeight - 1])
    rotate([0, 0, -90])
    triConnectionPoint();
    
    translate([8, depth - 10, topThickness+screenHeight - 1])
    rotate([0, 0, -90])
    triConnectionPoint();
    
    
    translate([width - extraSideWidth * 2 - 10, depth - 8, topThickness+screenHeight - 1])
    rotate([0, 0, 180])
    triConnectionPoint();
}


module triConnectionPoint(width = 10, depth = 8) {
    boxHeight = 5;
    prismHeight = height - topThickness - screenHeight - boxHeight + 1;
    
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

