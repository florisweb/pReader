
$fn=25;

wallThickness = 1.5;
topThickness = 0.7;

height = 10 + topThickness;
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



buttonSet();
translate([0, 20, 0])
buttonSet();
translate([0, 40, 0])
buttonSet();

module buttonSet() {
    translate([-5, 5, buttonWidth])
    rotate([0, 90, 0])
    button();

    difference() 
    {
        cube([10.7, 20, height]);
        translate([wallThickness, -5, topThickness])
        cube([30, 40, 12]);
        
        
        
        translate([
            wallThickness, 
            -buttonHoleMargin + 5, 
            -buttonHoleMargin + (height - topThickness) / 2 + topThickness - (buttonHeight + 2 *buttonHoleMargin) / 2])
        rotate([0, -90, 0])
        boxWithRoundedCorners(
            buttonHeight + buttonHoleMargin * 2, 
            buttonDepth + buttonHoleMargin * 2, 
            wallThickness, 
            .5
        );
    }
    translate([buttonClampWidth + TButtonPressableWidth + wallThickness, 7.5, topThickness])
    buttonHolder();
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