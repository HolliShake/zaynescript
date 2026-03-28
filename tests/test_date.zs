import { println } from "core:io";
import { Date } from "core:Date";


println("Testing Date module...");

// Test current date
const d1 = new Date();
println("Current Date:", d1.toString());

// Test string parsing
const dParsed = new Date("2023-12-25");
println("Parsed Date (2023-12-25):", dParsed.toString());
println("Year:", dParsed.getFullYear());   // Should be 2023
println("Month:", dParsed.getMonth());     // Should be 11
println("Day:", dParsed.getDate());        // Should be 25

const dParsed2 = new Date("2023/12/25 12:30:45");
println("Parsed Date 2 (2023/12/25 12:30:45):", dParsed2.toString());
println("Hours:", dParsed2.getHours());

const d2 = new Date(2023, 11, 25); 
println("Specific Date (2023-12-25):", d2.toString());
println("Year:", d2.getFullYear()); 
println("Month:", d2.getMonth());   
println("Day:", d2.getDate());      

const d3 = new Date(1705449600000); 
println("Timestamp Date:", d3.toString());
println("Time:", d3.getTime());     

const d4 = new Date(2024, 0, 1, 12, 30, 45);
println("Detailed Date (2024-01-01 12:30:45):", d4.toString());
println("Hours:", d4.getHours());
println("Minutes:", d4.getMinutes());
println("Seconds:", d4.getSeconds());

println("Done.");
