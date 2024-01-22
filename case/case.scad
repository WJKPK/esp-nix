// Define the dimensions of the case
$fn=30;
len = 100; // Length of the case
wid = 200; // Width of the case
dep = 100; // Depth of the case
wall = 3; // Thickness of the walls

module triangle_prism(b, d, h, ht) {
    linear_extrude(height = ht, scale = 1)
    polygon(points = [[0, 0], [b, 0], [d, h]]);
}

// Create the case
module case(len, wid, dep, wall) {
    minkowski_sphere_radius = 1;
    wall = wall - minkowski_sphere_radius;
    minkowski() {
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
                translate([wall, wall, 0])
                    cube([len -  wall, wid - 2 * wall, dep - wall]); // Inner box
            }
        }
        sphere(2 * minkowski_sphere_radius);
    }
}

case(len, wid, dep, wall);