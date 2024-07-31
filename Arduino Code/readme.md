# Summary

Due to the easy nature of this project, the Arduino environment has been chosen to code in. 
The code can be adapted quite easily and is very accessible because of this.

I have commented most lines in the code with an explanation of what it does.
This makes it also easy to alter for different values / types of cars which have a similar approach in their SWC. 

When it comes to sending a new voltage / resistance to a headunit, please keep in mind that this is the approach for at least Sony and Pioneer, two of the larger headunit manufacturers. Kenwood for instance has a different approach using IR signals over a single line. In that case, you can use the "analogRead" part of this code to at least detect buttonpresses, and adapt the output accordingly. 
