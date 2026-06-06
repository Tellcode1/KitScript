fn main() {
   assert io::exists("LICENSE");
   println("Assertion passed: io::exists(LICENSE) (This is user code btw)");

   assert(1 + 2 == 3);
   println("Assertion passed: 1 + 2 == 3 (Again, not from the compiler)");

   assert io::exists("LICENSEohnowhoaddedtheseextracharacters");
}