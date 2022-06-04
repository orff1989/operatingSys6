#include <iostream>

template <typename T>
class singleton{

private:
    static T* ins;

    singleton(){ }

public:
    static T* Instance(){
        if(ins==NULL)
            ins= new T();
        return ins;
    }

    static void Destroy();
};

template<typename T>
T* singleton<T>::ins = 0;

template<typename T>
void singleton<T>::Destroy(){
    delete ins;
    singleton<T>::ins = 0;
}

int test_singleton()
{
    int *p1=singleton<int>::Instance();
    *p1=22;
    printf("%d \n",*p1);
    int *p2=singleton<int>::Instance();
    printf("%d \n",*p2);
    *p2=101;

    printf("%d \n",*p1);
    singleton<int>::Destroy();
    printf("%d \n",*p1);

    return 0;
}