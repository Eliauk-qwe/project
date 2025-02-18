char * removeDuplicateLetters(char * s){
    if (s == NULL || strlen(s) == 0) {
        return "";
    }
    if (strlen(s) == 1) {
        return s;
    }
    int len = (int)strlen(s);
    //注意，这里需要初始化为0
    char recode[26] = {0};
    
    for (int i=0; i<len; i++) {
        recode[s[i] - 'a']++;
    }
    
    char * stack = (char *)malloc(sizeof(char) * (len+1));
    int top = -1;
    
    int isExist;
    for (int i=0; i<len; i++) {
        isExist = 0;
        for (int j=0; j<=top; j++) {
            if (s[i] == stack[j]) {
                isExist = 1;
                break;
            }
        }
        
        if (isExist) {
            recode[s[i] - 'a']--;
        }else{
            while(top>-1 && stack[top] > s[i] && recode[stack[top] - 'a'] > 1) {
                //如果栈顶字符比当前大，并且后边还会出现
                recode[stack[top] - 'a']--;
                //出栈
                top--;
            }
            //入栈
            stack[++top] = s[i];
        }
    }
    stack[++top] = '\0';
    return stack;
}
