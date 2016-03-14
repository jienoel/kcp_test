//
//  test.cpp
//  cj
//
//  Created by apple on 16/3/14.
//  Copyright © 2016年 apple. All rights reserved.
//

#include <iostream>
#include "test.hpp"
#include <stdio.h>
#include <stdlib.h>
//#include "ikcp.cpp"

LatencySimulator *vnet;
int udp_output(const char *buf, int len, ikcpcb *kcp, void *user)
{
    int id = (int)(size_t) user;
    //int id = 1;
    vnet->send(id, buf, len);
    return 0;
}

void test(int mode)
{
    // ´´½¨Ä£ÄâÍøÂç£º¶ª°üÂÊ10%£¬Rtt 60ms~125ms
    vnet = new LatencySimulator(10, 60, 125);
    
    // ´´½¨Á½¸ö¶ËµãµÄ kcp¶ÔÏó£¬µÚÒ»¸ö²ÎÊý convÊÇ»á»°±àºÅ£¬Í¬Ò»¸ö»á»°ÐèÒªÏàÍ¬
    // ×îºóÒ»¸öÊÇ user²ÎÊý£¬ÓÃÀ´´«µÝ±êÊ¶
    ikcpcb *kcp1 = ikcp_create(0x11223344, (void*)0);
    ikcpcb *kcp2 = ikcp_create(0x11223344, (void*)1);
    
    // ÉèÖÃkcpµÄÏÂ²ãÊä³ö£¬ÕâÀïÎª udp_output£¬Ä£ÄâudpÍøÂçÊä³öº¯Êý
    kcp1->output = udp_output;
    kcp2->output = udp_output;
    
    IUINT32 current = iclock();
    IUINT32 slap = current + 20;
    IUINT32 index = 0;
    IUINT32 next = 0;
    IINT64 sumrtt = 0;
    int count = 0;
    int maxrtt = 0;
    
    // ÅäÖÃ´°¿Ú´óÐ¡£ºÆ½¾ùÑÓ³Ù200ms£¬Ã¿20ms·¢ËÍÒ»¸ö°ü£¬
    // ¶ø¿¼ÂÇµ½¶ª°üÖØ·¢£¬ÉèÖÃ×î´óÊÕ·¢´°¿ÚÎª128
    ikcp_wndsize(kcp1, 128, 128);
    ikcp_wndsize(kcp2, 128, 128);
    
    // ÅÐ¶Ï²âÊÔÓÃÀýµÄÄ£Ê½
    if (mode == 0) {
        // Ä¬ÈÏÄ£Ê½
        ikcp_nodelay(kcp1, 0, 10, 0, 0);
        ikcp_nodelay(kcp2, 0, 10, 0, 0);
    }
    else if (mode == 1) {
        // ÆÕÍ¨Ä£Ê½£¬¹Ø±ÕÁ÷¿ØµÈ
        ikcp_nodelay(kcp1, 0, 10, 0, 1);
        ikcp_nodelay(kcp2, 0, 10, 0, 1);
    }	else {
        // Æô¶¯¿ìËÙÄ£Ê½
        // µÚ¶þ¸ö²ÎÊý nodelay-ÆôÓÃÒÔºóÈô¸É³£¹æ¼ÓËÙ½«Æô¶¯
        // µÚÈý¸ö²ÎÊý intervalÎªÄÚ²¿´¦ÀíÊ±ÖÓ£¬Ä¬ÈÏÉèÖÃÎª 10ms
        // µÚËÄ¸ö²ÎÊý resendÎª¿ìËÙÖØ´«Ö¸±ê£¬ÉèÖÃÎª2
        // µÚÎå¸ö²ÎÊý ÎªÊÇ·ñ½ûÓÃ³£¹æÁ÷¿Ø£¬ÕâÀï½ûÖ¹
        ikcp_nodelay(kcp1, 1, 10, 2, 1);
        ikcp_nodelay(kcp2, 1, 10, 2, 1);
    }
    
    
    char buffer[2000];
    int hr;
    
    IUINT32 ts1 = iclock();
    
    while (1) {
        isleep(1);
        current = iclock();
        ikcp_update(kcp1, iclock());
        ikcp_update(kcp2, iclock());
        
        // Ã¿¸ô 20ms£¬kcp1·¢ËÍÊý¾Ý
        for (; current >= slap; slap += 20) {
            *(IUINT32*)(buffer + 0) = index++;
            *(IUINT32*)(buffer + 4) = current;
            
            // ·¢ËÍÉÏ²ãÐ­Òé°ü
            ikcp_send(kcp1, buffer, 8);
        }
        
        // ´¦ÀíÐéÄâÍøÂç£º¼ì²âÊÇ·ñÓÐudp°ü´Óp1->p2
        while (1) {
            hr = vnet->recv(1, buffer, 2000);
            if (hr < 0) break;
            // Èç¹û p2ÊÕµ½udp£¬Ôò×÷ÎªÏÂ²ãÐ­ÒéÊäÈëµ½kcp2
            ikcp_input(kcp2, buffer, hr);
        }
        
        // ´¦ÀíÐéÄâÍøÂç£º¼ì²âÊÇ·ñÓÐudp°ü´Óp2->p1
        while (1) {
            hr = vnet->recv(0, buffer, 2000);
            if (hr < 0) break;
            // Èç¹û p1ÊÕµ½udp£¬Ôò×÷ÎªÏÂ²ãÐ­ÒéÊäÈëµ½kcp1
            ikcp_input(kcp1, buffer, hr);
        }
        
        // kcp2½ÓÊÕµ½ÈÎºÎ°ü¶¼·µ»Ø»ØÈ¥
        while (1) {
            hr = ikcp_recv(kcp2, buffer, 10);
            // Ã»ÓÐÊÕµ½°ü¾ÍÍË³ö
            if (hr < 0) break;
            // Èç¹ûÊÕµ½°ü¾Í»ØÉä
            ikcp_send(kcp2, buffer, hr);
        }
        
        // kcp1ÊÕµ½kcp2µÄ»ØÉäÊý¾Ý
        while (1) {
            hr = ikcp_recv(kcp1, buffer, 10);
            // Ã»ÓÐÊÕµ½°ü¾ÍÍË³ö
            if (hr < 0) break;
            IUINT32 sn = *(IUINT32*)(buffer + 0);
            IUINT32 ts = *(IUINT32*)(buffer + 4);
            IUINT32 rtt = current - ts;
            
            if (sn != next) {
                // Èç¹ûÊÕµ½µÄ°ü²»Á¬Ðø
                printf("ERROR sn %d<->%d\n", (int)count, (int)next);
                return;
            }
            
            next++;
            sumrtt += rtt;
            count++;
            if (rtt > (IUINT32)maxrtt) maxrtt = rtt;
            
            printf("[RECV] mode=%d sn=%d rtt=%d\n", mode, (int)sn, (int)rtt);
        }
        if (next > 1000) break;
    }
    
    ts1 = iclock() - ts1;
    
    ikcp_release(kcp1);
    ikcp_release(kcp2);
    
    const char *names[3] = { "default", "normal", "fast" };
    printf("%s mode result (%dms):\n", names[mode], (int)ts1);
    printf("avgrtt=%d maxrtt=%d\n", (int)(sumrtt / count), maxrtt);
    printf("press enter to next ...\n");
    char ch; scanf("%c", &ch);
}

int main(int argc, const char * argv[])
{
    std::cout << "Hello, World!\n";
    test(0);	// Ä¬ÈÏÄ£Ê½£¬ÀàËÆ TCP£ºÕý³£Ä£Ê½£¬ÎÞ¿ìËÙÖØ´«£¬³£¹æÁ÷¿Ø
    test(1);	// ÆÕÍ¨Ä£Ê½£¬¹Ø±ÕÁ÷¿ØµÈ
    test(2);	// ¿ìËÙÄ£Ê½£¬ËùÓÐ¿ª¹Ø¶¼´ò¿ª£¬ÇÒ¹Ø±ÕÁ÷¿Ø
    return 0;
}