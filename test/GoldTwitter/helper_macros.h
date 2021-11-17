/* Helper Macros */



// 指定したスレーブの指定したindexの情報をSDO通信で取得し，bufに保存．
// また，保存した値をprintfで表示
#define READ(slaveId, idx, sub, buf, comment)    \
    {   \
        buf=0;  \
        int __s = sizeof(buf);    \
        int __ret = ec_SDOread(slaveId, idx, sub, FALSE, &__s, &buf, EC_TIMEOUTRXM);   \
        printf("Slave: %d - Read at 0x%04x:%d => wkc: %d; data: 0x%.*x (%d)\t[%s]\n", slaveId, idx, sub, __ret, __s, (unsigned int)buf, (unsigned int)buf, comment);    \
     }

#define WRITE(slaveId, idx, sub, buf, value, comment) \
    {   \
        int __s = sizeof(buf);  \
        buf = value;    \
        int __ret = ec_SDOwrite(slaveId, idx, sub, FALSE, __s, &buf, EC_TIMEOUTRXM);  \
        printf("Slave: %d - Write at 0x%04x:%d => wkc: %d; data: 0x%.*x\t{%s}\n", slaveId, idx, sub, __ret, __s, (unsigned int)buf, comment);    \
    }

#define CHECKERROR(slaveId)   \
{   \
    ec_readstate();\
    printf("EC> \"%s\" %x - %x [%s] \n", (char*)ec_elist2string(), ec_slave[slaveId].state, ec_slave[slaveId].ALstatuscode, (char*)ec_ALstatuscode2string(ec_slave[slaveId].ALstatuscode));    \
}


// SDO通信に使用する関数の詳細は以下の通り(ethercatcoe.c)
/** CoE SDO read, blocking. Single subindex or Complete Access.
 *
 * Only a "normal" upload request is issued. If the requested parameter is <= 4bytes
 * then a "expedited" response is returned, otherwise a "normal" response. If a "normal"
 * response is larger than the mailbox size then the response is segmented. The function
 * will combine all segments and copy them to the parameter buffer.
 *
 * @param[in]  slave      = Slave number
 * @param[in]  index      = Index to read
 * @param[in]  subindex   = Subindex to read, must be 0 or 1 if CA is used.
 * @param[in]  CA         = FALSE = single subindex. TRUE = Complete Access, all subindexes read.
 * @param[in,out] psize   = Size in bytes of parameter buffer, returns bytes read from SDO.
 * @param[out] p          = Pointer to parameter buffer
 * @param[in]  timeout    = Timeout in us, standard is EC_TIMEOUTRXM
 * @return Workcounter from last slave response
 * @see ecx_SDOread

int ec_SDOread(uint16 slave, uint16 index, uint8 subindex,
                boolean CA, int *psize, void *p, int timeout)
{
   return ecx_SDOread(&ecx_context, slave, index, subindex, CA, psize, p, timeout);
}
*/

/** CoE SDO write, blocking. Single subindex or Complete Access.
 *
 * A "normal" download request is issued, unless we have
 * small data, then a "expedited" transfer is used. If the parameter is larger than
 * the mailbox size then the download is segmented. The function will split the
 * parameter data in segments and send them to the slave one by one.
 *
 * @param[in]  Slave      = Slave number
 * @param[in]  Index      = Index to write
 * @param[in]  SubIndex   = Subindex to write, must be 0 or 1 if CA is used.
 * @param[in]  CA         = FALSE = single subindex. TRUE = Complete Access, all subindexes written.
 * @param[in]  psize      = Size in bytes of parameter buffer.
 * @param[out] p          = Pointer to parameter buffer
 * @param[in]  Timeout    = Timeout in us, standard is EC_TIMEOUTRXM
 * @return Workcounter from last slave response
 * @see ecx_SDOwrite

int ec_SDOwrite(uint16 Slave, uint16 Index, uint8 SubIndex,
                boolean CA, int psize, void *p, int Timeout)
{
   return ecx_SDOwrite(&ecx_context, Slave, Index, SubIndex, CA, psize, p, Timeout);
}
*/