# CN_CHomeworks_2
### ns3 Building The Project ####

first go to the `ns-3.35` using the `cd` command in UNIX based Devices.
Using the `./waf` command followed by `build` you can build all the files in the know directories.

### Running Simulation ###
You can use the `./waf` followed by `--run` and the name of your cpp project excluding its extension(,.ie .cc).
So your command should look something like this
> ./waf --run sample:

### Adding Mapper ###
A class called **mapper** was added to the project.
**Two** sockets must be installed in each _mapper_.
One of which is a _UDP_ socket for sending packets to **client**.
The other one is a _TCP_ socket for receiving packets from **master**.

## Data Mapping ##
Received data from the master node will be sent to all of the mapper nodes.
From there a **hard coded** table will be initialized for each mapper, and the corresponding mapper will send appropriate mapped data to the client.

## Throughput ##
_Throuput_ by definition is the **observed** rate at which data are sent through a channel.
In this project, the throughput have been calculated for each flow.

## Averegae end-to-end delay ##
The _delay_ corresponds to how long it takes a message to travel from one end of a network to another.
Here just like _Throughput_ the calculations are printed for each flow.

## The Hard Coded Table ##

![Screenshot from 2023-04-20 21-43-12](https://user-images.githubusercontent.com/88041997/233453112-e3ab9029-b8f1-4f3c-89e2-bdd992d3df4d.png)
 
 ## Corresponding Result ##
![Screenshot from 2023-04-20 21-45-39](https://user-images.githubusercontent.com/88041997/233453155-4beef10f-7bc9-4199-aabe-c91d152bac65.png)
