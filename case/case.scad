// Define the dimensions of the casket
len = 100; // Length of the casket
wid = 100; // Width of the casket
dep = 100; // Depth of the casket
wall = 5; // Thickness of the walls

module triangle_prism(b, d, h, ht) {
    linear_extrude(height = ht, scale = 1)
    polygon(points = [[0, 0], [b, 0], [d, h]]);
}

// Create the casket
union() {
    difference() {
        translate([len, wid, 0])
            rotate([90,0,0])
                triangle_prism(100,0,100,wid);
        translate([len - wall, wid - wall, 0])
            rotate([90,0,0])
                triangle_prism(100,0,100,wid-2*wall);
    }
    difference() {
        cube([len, wid, dep]); // Outer box
        translate([wall, wall, -wall])
            cube([len - 2 * wall, wid - 2 * wall, dep - wall]); // Inner box
    }
}
