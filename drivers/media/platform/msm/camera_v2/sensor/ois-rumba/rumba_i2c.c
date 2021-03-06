#include "utils.h" //type def uintxx in msm_ois.h
#include "rumba_i2c.h"

#undef  pr_fmt
#define pr_fmt(fmt) " OIS-I2C %s() " fmt, __func__

int rumba_read_byte(struct msm_ois_ctrl_t * ctrl,uint16_t reg_addr, uint16_t* reg_data)
{
	int rc = 0;
	int i;
	struct msm_ois_ctrl_t *msm_ois_t_pointer = ctrl;

	for(i=0;i<3;i++)
	{
		rc = msm_ois_t_pointer->i2c_client.i2c_func_tbl->i2c_read(&(msm_ois_t_pointer->i2c_client),
				reg_addr,
				reg_data,
				MSM_CAMERA_I2C_BYTE_DATA);
		if(rc == 0 || rc == -110)
		{
			break;
		}
		else
		{
			pr_err("read reg 0x%04x failed, rc = %d, retry_count = %d\n",reg_addr,rc,i);
			delay_ms(2);
		}
	}

	if( i != 0 )
	{
		if(rc == 0)
		{
			pr_info("read reg 0x%04x OK after retried %d times\n",reg_addr,i);
		}
		else
		{
			pr_info("read reg 0x%04x FAILED after retried %d times\n",reg_addr,i);
		}
	}
	if(rc == -110)
	{
		pr_info("read reg 0x%04x FAILED due to cci timeout!\n",reg_addr);
	}
	return rc;
}

int rumba_read_word(struct msm_ois_ctrl_t * ctrl,uint16_t reg_addr, uint16_t* reg_data)
{
	int rc = 0;
	int i;
	struct msm_ois_ctrl_t *msm_ois_t_pointer = ctrl;

	for(i=0;i<3;i++)
	{
		rc = msm_ois_t_pointer->i2c_client.i2c_func_tbl->i2c_read(&(msm_ois_t_pointer->i2c_client),
				reg_addr,
				reg_data,
				MSM_CAMERA_I2C_WORD_DATA);
		if(rc == 0 || rc == -110)
		{
			//swap_word_data(reg_data);
			break;
		}
		else
		{
			pr_err("read reg 0x%04x failed, rc = %d, retry_count = %d\n",reg_addr,rc,i);
			delay_ms(2);
		}
	}

	if( i != 0 )
	{
		if(rc == 0)
		{
			pr_info("read reg 0x%04x OK after retried %d times\n",reg_addr,i);
		}
		else
		{
			pr_info("read reg 0x%04x FAILED after retried %d times\n",reg_addr,i);
		}
	}
	if(rc == -110)
	{
		pr_info("read reg 0x%04x FAILED due to cci timeout!\n",reg_addr);
	}

	return rc;
}
int rumba_read_dword(struct msm_ois_ctrl_t * ctrl,uint16_t reg_addr, uint32_t* reg_data)
{
	int rc = 0;
	struct msm_ois_ctrl_t *msm_ois_t_pointer = ctrl;
	struct msm_camera_i2c_seq_reg_array *reg_setting = NULL;

	reg_setting = kzalloc(sizeof(struct msm_camera_i2c_seq_reg_array),GFP_KERNEL);
	if (!reg_setting)
		return -ENOMEM;

	reg_setting->reg_addr = reg_addr;
	reg_setting->reg_data_size = 4;

	rc = msm_ois_t_pointer->i2c_client.i2c_func_tbl->i2c_read_seq(&msm_ois_t_pointer->i2c_client,
			reg_setting->reg_addr,
			reg_setting->reg_data,
			reg_setting->reg_data_size);
	if(rc < 0)
	{
		pr_err("read reg 0x%04x failed, rc = %d\n",reg_addr,rc);
	}
	//MSB in higher address
	*reg_data = ((uint32_t)(reg_setting->reg_data[3] <<24)|
				 (uint32_t)(reg_setting->reg_data[2] <<16)|
				 (uint32_t)(reg_setting->reg_data[1] <<8)|
				 (uint32_t)(reg_setting->reg_data[0] )
				);
	pr_info("read 0x%x from reg 0x%04x, seq_value %x %x %x %x\n",
			*reg_data,
			reg_setting->reg_addr,
			reg_setting->reg_data[0],
			reg_setting->reg_data[1],
			reg_setting->reg_data[2],
			reg_setting->reg_data[3]
		  );
	kfree(reg_setting);
	reg_setting = NULL;
	return rc;
}
int rumba_read_seq_bytes(struct msm_ois_ctrl_t * ctrl,uint16_t reg_addr, uint8_t* reg_data, uint32_t size)
{
	int rc = 0;
	struct msm_ois_ctrl_t *msm_ois_t_pointer = ctrl;
	struct msm_camera_i2c_seq_reg_array *reg_setting = NULL;
	int i;

	if(size > I2C_SEQ_REG_DATA_MAX)
	{
		pr_err("read size %d too large, max seq size is %d\n",size,I2C_SEQ_REG_DATA_MAX);
		return -1;
	}

	reg_setting = kzalloc(sizeof(struct msm_camera_i2c_seq_reg_array),GFP_KERNEL);
	if (!reg_setting)
		return -ENOMEM;

	reg_setting->reg_addr = reg_addr;
	reg_setting->reg_data_size = size;

	rc = msm_ois_t_pointer->i2c_client.i2c_func_tbl->i2c_read_seq(&msm_ois_t_pointer->i2c_client,
			reg_setting->reg_addr,
			reg_setting->reg_data,
			reg_setting->reg_data_size);
	if(rc < 0)
	{
		pr_err("read reg 0x%04x failed, rc = %d\n",reg_addr,rc);
	}

	for(i=0;i<size;i++)
	{
		reg_data[i] = reg_setting->reg_data[i];
	}

	kfree(reg_setting);
	reg_setting = NULL;
	return rc;
}

int rumba_poll_byte(struct msm_ois_ctrl_t * ctrl,uint16_t reg_addr, uint16_t reg_data, uint32_t delay_ms)
{
	int rc = 0;
	struct msm_ois_ctrl_t *msm_ois_t_pointer = ctrl;

	rc = msm_ois_t_pointer->i2c_client.i2c_func_tbl->i2c_poll(&msm_ois_t_pointer->i2c_client,
					reg_addr,
					reg_data,
					MSM_CAMERA_I2C_BYTE_DATA,
					delay_ms);
	if(rc != 0)
	{
		pr_err("poll 0x%x from reg 0x%04x delay %d ms failed, rc = %d\n",reg_data,reg_addr,delay_ms,rc);
		rc = rumba_read_byte(msm_ois_t_pointer,reg_addr,&reg_data);
		if(rc < 0)
		{
			//
		}
		else
		{
			pr_info("reg 0x%04x value now is 0x%x actually\n",reg_addr,reg_data);
		}
	}

	return rc;
}

int rumba_poll_word(struct msm_ois_ctrl_t * ctrl,uint16_t reg_addr, uint16_t reg_data, uint32_t delay_ms)
{
	int rc = 0;
	struct msm_ois_ctrl_t *msm_ois_t_pointer = ctrl;

	swap_word_data(&reg_data);

	rc = msm_ois_t_pointer->i2c_client.i2c_func_tbl->i2c_poll(&msm_ois_t_pointer->i2c_client,
					reg_addr,
					reg_data,
					MSM_CAMERA_I2C_WORD_DATA,
					delay_ms);
	if(rc != 0)
	{
		pr_err("poll 0x%x from reg 0x%04x delay %d ms failed, rc = %d\n",reg_data,reg_addr,delay_ms,rc);
		rc = rumba_read_word(msm_ois_t_pointer,reg_addr,&reg_data);
		if(rc < 0)
		{
			//
		}
		else
		{
			pr_info("reg 0x%04x value now is 0x%x actually\n",reg_addr,reg_data);
		}

	}

	return rc;
}

int rumba_write_byte(struct msm_ois_ctrl_t * ctrl,uint16_t reg_addr, uint16_t reg_data)
{
	int rc = 0;
	struct msm_ois_ctrl_t *msm_ois_t_pointer = ctrl;

	rc = msm_ois_t_pointer->i2c_client.i2c_func_tbl->i2c_write(&(msm_ois_t_pointer->i2c_client),
			reg_addr,
			reg_data,
			MSM_CAMERA_I2C_BYTE_DATA);
	if(rc < 0)
	{
		pr_err("write 0x%x to reg 0x%04x failed, rc = %d\n",reg_data,reg_addr,rc);
	}
	return rc;
}
int rumba_write_word(struct msm_ois_ctrl_t * ctrl,uint16_t reg_addr, uint16_t reg_data)
{
	int rc = 0;
	struct msm_ois_ctrl_t *msm_ois_t_pointer = ctrl;

	//swap_word_data(&reg_data);
	rc = msm_ois_t_pointer->i2c_client.i2c_func_tbl->i2c_write(&(msm_ois_t_pointer->i2c_client),
			reg_addr,
			reg_data,
			MSM_CAMERA_I2C_WORD_DATA);
	if(rc < 0)
	{
		pr_err("write 0x%x to reg 0x%04x failed, rc = %d\n",reg_data,reg_addr,rc);
	}
	return rc;
}
int rumba_write_dword(struct msm_ois_ctrl_t * ctrl,uint16_t reg_addr, uint32_t reg_data)
{
	int rc = 0;
	struct msm_ois_ctrl_t *msm_ois_t_pointer = ctrl;
	struct msm_camera_i2c_seq_reg_array *reg_setting = NULL;

	reg_setting = kzalloc(sizeof(struct msm_camera_i2c_seq_reg_array),GFP_KERNEL);
	if (!reg_setting)
		return -ENOMEM;

	reg_setting->reg_addr = reg_addr;
	reg_setting->reg_data_size = 4;

	//MSB in higher address
	reg_setting->reg_data[0] = (uint8_t)((reg_data & 0xFF000000) >> 24);
	reg_setting->reg_data[1] = (uint8_t)((reg_data & 0x00FF0000) >> 16);
	reg_setting->reg_data[2] = (uint8_t)((reg_data & 0x0000FF00) >> 8);
	reg_setting->reg_data[3] = (uint8_t)(reg_data & 0x000000FF);

	rc = msm_ois_t_pointer->i2c_client.i2c_func_tbl->i2c_write_seq(&msm_ois_t_pointer->i2c_client,
			reg_setting->reg_addr,
			reg_setting->reg_data,
			reg_setting->reg_data_size);
	if(rc < 0)
	{
		pr_err("write 0x%x to reg 0x%04x failed, rc = %d\n",reg_data,reg_addr,rc);
	}

	/*pr_info("write 0x%x to reg 0x%04x, seq_value %x %x %x %x\n",
			reg_data,
			reg_setting->reg_addr,
			reg_setting->reg_data[0],
			reg_setting->reg_data[1],
			reg_setting->reg_data[2],
			reg_setting->reg_data[3]
		  );*/
	kfree(reg_setting);
	reg_setting = NULL;
	return rc;
}
int rumba_write_seq_bytes(struct msm_ois_ctrl_t * ctrl,uint16_t reg_addr, uint8_t* reg_data,uint32_t size)
{
	int rc = 0;
	struct msm_ois_ctrl_t *msm_ois_t_pointer = ctrl;
	struct msm_camera_i2c_seq_reg_array *reg_setting = NULL;
	int i;

	if(size > I2C_SEQ_REG_DATA_MAX)
	{
		pr_err("write size %d too large, max seq size is %d\n",size,I2C_SEQ_REG_DATA_MAX);
		return -1;
	}

	reg_setting = kzalloc(sizeof(struct msm_camera_i2c_seq_reg_array),GFP_KERNEL);
	if (!reg_setting)
		return -ENOMEM;

	reg_setting->reg_addr = reg_addr;
	reg_setting->reg_data_size = size;

	for(i=0;i<size;i++)
	{
		reg_setting->reg_data[i] = reg_data[i];
	}


	rc = msm_ois_t_pointer->i2c_client.i2c_func_tbl->i2c_write_seq(&msm_ois_t_pointer->i2c_client,
			reg_setting->reg_addr,
			reg_setting->reg_data,
			reg_setting->reg_data_size);
	if(rc < 0)
	{
		pr_err("write seq bytes[%d] to reg 0x%04x failed, rc = %d\n",size-1,reg_addr,rc);
	}

	kfree(reg_setting);
	reg_setting = NULL;
	return rc;
}
