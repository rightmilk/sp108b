int power(int a, int b){
    int ans = 1;
    int i;
    for (i = 0; i < b; i++){
        ans *= a;
    }
    return ans;
}