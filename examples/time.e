fn main() {
   let timestamp = time::local();

   let day_table = #{ 1 : "Sunday", 2 : "Monday", 3 : "Tuesday", 4 : "Wednesday", 5 : "Thursday", 6 : "Friday", 7 : "Saturday" };

   // Month starts from 1, so add a null variable to the beginning
   let month_table = [null, "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"];

   println("Today is: ", day_table[timestamp.wday]);
   println("The month is: ", month_table[timestamp.mon]);
   println("And the year is: ", timestamp.year);

   println("Or, todays date is: ", day_table[timestamp.wday], ", ", timestamp.day, " ", month_table[timestamp.mon], ", ", timestamp.year);

   io::println(io::STDOUT, "Pinging the monotonic timer: ", time::mono());

   print("Waiting for 1 second to pass (busy wait): ");
   io::flush(io::STDOUT);

   let start = time::mono();
   while ((time::mono() - start) < 1);

   println("Done!");

   print("Waiting for 1 second to pass (OS request): ");
   io::flush(io::STDOUT);

   sys::sleepms(1000);

   println("Done!");
}