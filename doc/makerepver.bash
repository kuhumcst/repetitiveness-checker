METH=git
METH=https

if [ ! -d letterfunc ]; then
    mkdir letterfunc
    cd letterfunc
    git init
    git remote add origin $METH://github.com/kuhumcst/letterfunc.git
    cd ..
fi
cd letterfunc
git pull origin master
cd ..

if [ ! -d repetitiveness-checker ]; then
    mkdir repetitiveness-checker 
    cd repetitiveness-checker 
    git init
    git remote add origin $METH://github.com/kuhumcst/repetitiveness-checker.git
    cd ..
fi
cd repetitiveness-checker
git pull origin master
cd src
make all
cd ..
cd ..
