# yaml-parser
yaml parser based on libyaml

## Prerequisites
First install libyaml-dev

### for Ubuntu
```
apt install libyaml-dev
```


### for centos 7
```
yum install libyaml-devel
```

### for SLES,
```
zypper addrepo https://download.opensuse.org/repositories/devel:libraries:c_c++/openSUSE_Leap_15.3/devel:libraries:c_c++.repo
zypper refresh
zypper install libyaml-devel
```

## Building
git clone https://github.com/manoj-freyr/yaml-parser.git
cd yaml-parser
mkdir build
cd build
cmake ..
make 

## Run
./helloDemo
