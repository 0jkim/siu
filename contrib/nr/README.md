# 3GPP NR ns-3 module #

This is an [ns-3](https://www.nsnam.org "ns-3 Website") 3GPP NR module for the
simulation of NR non-standalone cellular networks. Ns-3 is used as a base,
on top of which we will add our module as plug-in (with limitations that will
be discussed below).


### Clone the NS3 and NR repos in your computer: ns-3-dev and nr

Create a folder locally:

```
$ mkdir yourFolder
```
Clone the ns-3-dev with the needed changes in order to use CG:

```
$ git clone git@gitlab.com:ns-3-dev-nr-configuredgrant/ns-3-dev.git
```

Go to the contrib folder:
```
$ cd ns-3-dev/contrib
```
Clone the nr repository:
```
$ git clone git@gitlab.com:ns-3-dev-nr-configuredgrant/nr.git
```
We now have all the content locally. Let's move to the branches we are interested in.


### Checkout in the CG branch: 5g-lena-cg-v2.1.y

We are in the contrib folder, checkout in the CG branch:
```
$ git checkout remotes/origin/5g-lena-cg-v2.1.y
$ git checkout -b 5g-lena-cg-v2.1.y
```

### Checkout in the NS-3-DEV with CG branch: remotes/origin/ns-3.36-cg

Go to ns-3-dev folder and checkout in the ns-3-dev CG branch:
```
$ cd ../..
$ git checkout remotes/origin/ns-3.36-cg
$ git checkout -b ns-3.36-cg
```

### Create a group on your remote-git with two projects (ns-3-dev and nr)
Create group

### Push the code to these projects

```
$ git push git@gitlab.com:yourGroup/project_1NS3DEV.git ns-3.36-cg
$ cd contrib/nr
$ git push git@gitlab.com:yourGroup/project_2NR.git 5g-lena-cg-v2.1.y 
```

### Test the NR installation

Let's configure the project:

```
$ cd ../..
$ ./ns3 configure --enable-examples --enable-tests
```

If the NR module is recognized correctly, you should see "nr" in the list of
built modules. If that is not the case, then most probably the previous
point failed. Otherwise, you could compile it:

```
$ ./ns3
```

If that command returns successfully, Welcome to the NR world !

Notice that sqlite development package and semaphore.h are required (otherwise
you will get an error, e.g: `fatal error: ns3/sqlite-output.h`). In this case
you should install libc6-dev:

```
sudo apt-get install libc6-dev
```

that will provide semaphore.h and/or sqlite:

```
apt-get install sqlite sqlite3 libsqlite3-dev
```

For more details, related to the prerequisites for ns-3 please visit: `https://www.nsnam.org/wiki/Installation#Ubuntu.2FDebian.2FMint`.
After the installation of the missing packages run again `./ns3 configure --enable-tests --enable-examples`.
You should see: `SQLite stats support: enabled`


## Building **Configured Grant** and **URLLC schedulers** for UL periodic transmissions code
At this point we have the ns-3-dev repository installed and we are on version 3.36, 
as well as 5g-lena cloned inside contrib folder. We are also checkout on the branches we are interested in.

To test the configured grant scheduling with the new scheudlers,
we are going to use the file inside Build_CG named test_configuredGrant.cc
We are going to save this file in the scratch folder.
Then we are going to create a log to see a simple configuration of configured grant.
```
export 'NS_LOG=ConfiguredGrant=level_all|prefix_func|prefix_time:NrUePhy=level_all|prefix_func|prefix_time:NrUeMac=level_all|prefix_func|prefix_time:NrMacSchedulerNs3=level_all|prefix_func|prefix_time:LteRlcUm=level_all|prefix_func|prefix_time:NrGnbPhy=level_all|prefix_func|prefix_time:NrGnbMac=level_all|prefix_func|prefix_time:NrMacSchedulerOfdma=level_all|prefix_func|prefix_time:NrSpectrumPhy=level_all|prefix_func|prefix_time:BwpManagerGnb=level_all|prefix_func|prefix_time:NrMacSchedulerHarqRr=level_all|prefix_func|prefix_time' 

./ns3 run scratch/ConfiguredGrant_firstTest > ConfiguredGrant_firstTest.out 2>&1
```

## Documentation

We maintain two sources of documentation: a user manual, and the Doxygen API
documentation. The user manual describes the models and their assumptions; as
we developed the module while the standard was not fully available, some parts
are not modeling precisely the bits and the procedures indicated by the
standard. However, we tried to abstract them accurately. In the Doxygen API
documentation, you will find details about design and user usage of any class
of the module, as well as description and images for the examples and the
tests.

To build the user manual, please do:

```
$ cd doc
$ make latexpdf
```

And you fill find the PDF user manual in the directory build/latex. Please note
that you may have to install some requirements to build the documentation; you
can find the list of packages for any Ubuntu-based distribution in the file
`.gitlab-ci.yml`.

To build the doxygen documentation, please do:

```
$ python3 doc/m.css/documentation/doxygen.py doc/doxygen-mcss.conf --debug
```

And then you will find the doxygen documentation inside `doc/doc/html/`.
Please note that you may need to initialize the m.css submodule, and
to install some packages like python3.

## Features

To see the features, please go to the [official webpage](https://cttc-lena.gitlab.io/5g-lena-website/features/).

## Papers

An updated list of published papers that are based on the outcome of this
module is available
[here](https://cttc-lena.gitlab.io/5g-lena-website/papers/).

## Future work

## About

The Mobile Networks group in CTTC is a group of 10 highly skilled researchers, with expertise in the area of mobile and computer networks, ML/AI based network management, SDN/NFV, energy management, performance evaluation. Our work on performance evaluation started with the design and development of the LTE module of ns-3.

We are [on the web](https://cttc-lena.gitlab.io/5g-lena-website/about/).

## 5G-LENA Authors ##

In alphabetical order:

- Zoraze Ali
- Biljana Bojovic
- Lorenza Giupponi
- Katerina Koutlia
- Sandra Lagen
- Natale Patriciello

Inspired by [mmWave module by NYU/UniPD] (https://github.com/nyuwireless-unipd/ns3-mmwave)

## Configured Grant in 5G-LENA Author ##

Ana Larrañaga

## License ##

This software is licensed under the terms of the GNU GPLv2, as like as ns-3.
See the LICENSE file for more details.
