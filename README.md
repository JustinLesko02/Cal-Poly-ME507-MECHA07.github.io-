### Hello! Welcome to our Mechatronics Term Project.  Created by Evan Long and Justin Lesko.

This repository hosts the software we developed for our term project for Cal Poly's ME507 Mechatronics class.  The overall 
 goal of the project was to select a personal project that requires automation and design a custom PCB to accomplish the task.
 Our project is an automatic cat feeder.  The user will place a large quantity of dry cat food in the storage container
 of the feeder, and the feeder will automatically dispense the food at the appropriate time for the cat.  Over the course
 of the spring 2025 quarter, we designed the cat feeder and custom PCB, ordered the parts, wired everything up, and 
 programmed the feeder to create a functional final product.

The cat feeder dispenses food with a large lead screw, and has a pair of load cells to measure the weight of food in the
 bowl.  The feeder is equiped with a wifi module and is programmed to act as an http server.  With port forwarding set up
 on a local network, the user can use any browser to access the feeder controls from anywhere in with internet access.
 As a result, the user can be out of town and still log in to feed the cat.

This github repository serves two purposes.  One, it hosts our website, which contains an overview of the mechanical,
electrical, and software designs for this project.  Two, it contains our source code, in a folder called "ME507 Code".

A link to our website can be found here: [WEBSITE](https://cal-poly-me507-mecha07.github.io/)