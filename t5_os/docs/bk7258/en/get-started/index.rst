Quick Start Guide
==============================================

:link_to_translation:`zh_CN:[中文]`



Armino AVDK SDK Code download
--------------------------------------------------------------------

We can download Armino AVDK SDK from gitlab::

    mkdir -p ~/armino
    cd ~/armino
    git clone http://gitlab.bekencorp.com/armino/bk_avdk.git
	

We also can download Armino AVDK SDK from github::

	mkdir -p ~/armino
	cd ~/armino
	git clone --recurse-submodules https://github.com/bekencorp/bk_avdk.git 
	

Then switch to the stable branch Tag node, such as v2.0.1.8::

    cd ~/armino/bk_avdk
    git checkout -B your_branch_name v2.0.1.8
    git submodule update --init --recursive
    ls
    
.. figure:: ../../_static/bk_avdk_dir_1.png
    :align: center
    :alt: advk dir
    :figclass: align-center

    Figure 1. AVDK Directiry structure

!Note:

    The latest SDK code is downloaded from gitlab on the official website, and
	relevant accounts can be found on the project to review the application.


Build Compilation Environment:
--------------------------------------------------------------------

.. note::

    Armino, currently supports compiling in Linux environment. This chapter willtake Ubuntu 20.04 LTS
	as an example to introduce the construction of the entire compiling environment.
    

Install Tool Chain
*************************************

Click `Download <https://dl.bekencorp.com/tools/toolchain/arm/gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2>`_ to download the BK7258 toolchain.

After downloading the tool kit, decompress it to '/opt/'::

    $ sudo tar -xvjf gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2 -C /opt/


.. note::

    Tool chain the default path is configured in the middleware/soc/bk7258/bk7258.defconfig, you can modify ``CONFIG_TOOLCHAIN_PATH`` to set to your owner toolchain path:

        CONFIG_TOOLCHAIN_PATH="/opt/gcc-arm-none-eabi-10.3-2021.10/bin"


Install Depended libraries
*************************************

Enter the following command in the terminal to install python3,CMake,Ninja::

    sudo dpkg --add-architecture i386
    sudo apt-get update
    sudo apt-get install build-essential cmake python3 python3-pip doxygen ninja-build libc6:i386 libstdc++6:i386 libncurses5-dev lib32z1 -y
    sudo pip3 install pycrypto click

Install python dependencies
*************************************

Enter the following command to install python dependencies::

    sudo pip3 install sphinx_rtd_theme future breathe blockdiag sphinxcontrib-seqdiag sphinxcontrib-actdiag sphinxcontrib-nwdiag sphinxcontrib.blockdiag


If you default Python is Python2, please set it to Python3::

    sudo ln -s /usr/bin/python3 /usr/bin/python


Build The Project
------------------------------------

Run following commands to build BK7258 default doorbell project::

    cd ~/armino/bk_avdk
    make bk7258


You can also build projects with PROJECT parameter, e.g. run "make bk7258 PROJECT=media/doorbell" 
can build projects/media/doorbell etc.

Configuration project
------------------------------------

We can also use the project configuration file for differentiated configuration::

    Project Profile Override Chip Profile Override Default Configuration
    Example： config >> bk7258.defconfig >> KConfig
    + Example of project configuration file：
        projects/media/doorbell/config/bk7258/config
    + Sample chip configuration file：
        middleware/soc/bk7258/bk7258.defconfig
    + Sample KConfig configuration file：
        middleware/arch/cm33/Kconfig
        components/bk_cli/Kconfig


Create New project
------------------------------------

The default project is projects/media/doorbell. For new projects, please refer to the project in projects/media/


Burn Code
------------------------------------

On the Windows platform, Armino currently supports UART burning.

For detailed `burning process <https://docs.bekencorp.com/arminodoc/bk_idk/bk7258/en/v2.0.1/get-started/index.html>`_, please refer to `IDK <https://docs.bekencorp.com/arminodoc/bk_idk/bk7258/en/v2.0.1/index.html>`_
