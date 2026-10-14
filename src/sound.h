void sound_init(void);
void sound_set_period(int period);
void sound_set_clock(int clock_mhz);
void sound_update_filter(void);
void sound_enable(int enable);
void sound_dump_audio(double delay, double duration);

/*Audio dump states, as returned by sound_get_dump_state()*/
enum
{
	AUDIO_DUMP_OFF = 0,
	AUDIO_DUMP_WAITING,
	AUDIO_DUMP_RECORDING,
	AUDIO_DUMP_FINISHED
};
int sound_get_dump_state(void);


extern int sound_gain;
extern int sound_filter;
