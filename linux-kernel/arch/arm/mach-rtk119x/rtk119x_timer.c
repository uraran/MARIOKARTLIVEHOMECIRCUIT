#include <linux/clockchips.h>
#include <linux/clocksource.h>
#include <linux/interrupt.h>
#include <linux/irq.h>

#define TIMER0				0
#define TIMER1				1
#define TIMER2				2
#define TIMER_MAX			(TIMER2 + 1)

#define SYSTEM_TIMER		TIMER0

#define COUNTER				0
#define TIMER				1

#define TIMERINFO_TIMER_ENABLE		(1<<31)
#define TIMERINFO_TIMER_MODE		(1<<30)
#define TIMERINFO_TIMER_PAUSE		(1<<29)
#define TIMERINFO_INTERRUPT_ENABLE	(1<<31)

/*! HW timer command enum description */
enum hwtimer_commands {
	HWT_START		= 0x80,		/*!< Start a timer/counter */
	HWT_STOP,					/*!< Stop a timer/counter */
	HWT_PAUSE,					/*!< Pause a timer/counter */
	HWT_RESUME,					/*!< Restart a timer/counter */
	HWT_INT_ENABLE,				/*!< Enable timer/counter interrupt */
	HWT_INT_DISABLE,				/*!< Disable timer/counter interrupt */
	HWT_INT_CLEAR,				/*!< Clear timer/counter interrupt pending bit */
};

int UMSK_TC_shift[3] = {
	1<<6,
	1<<7,
	0,
};

static irqreturn_t rtk119x_clock_event_isr(int, void*);
static int rtk_clkevt_set_next(int index, unsigned long, struct clock_event_device*);
static void rtk_clkevt_set_mode(int index, enum clock_event_mode, struct clock_event_device*);
int create_timer(unsigned char id, unsigned int interval, unsigned char mode);
int timer_control(unsigned char id, unsigned int cmd);
int timer_get_value(unsigned char id);
int timer_set_value(unsigned char id, unsigned int value);
int timer_set_target(unsigned char id, unsigned int value);
static cycle_t rtk_read_sched_clock(struct clocksource *cs);

struct _suspend_data {
    unsigned int    value;
    unsigned char   mode;
};

static struct _suspend_data sTimerSuspendData[TIMER_MAX];

static void __iomem *timer_base;
unsigned long	clk_freq;

#define MISC_IO_ADDR(pa)	(timer_base + pa)

#define rtd_setbits(offset, Mask)	__raw_writel(((__raw_readl(offset) | Mask)), offset)
#define rtd_clearbits(offset, Mask)	__raw_writel(((__raw_readl(offset) & ~Mask)), offset)

unsigned char timer_get_mode(unsigned char id);

int timer_get_value(unsigned char id)
{
	// get the current timer's value
	return __raw_readl(MISC_IO_ADDR(TCCVR_OFFSET + (id<<2)));
}

int timer_set_value(unsigned char id, unsigned int value)
{
	// set the timer's initial value
	__raw_writel(value, MISC_IO_ADDR(TCCVR_OFFSET + (id<<2)));

	return 0;
}

int timer_set_target(unsigned char id, unsigned int value)
{
	// set the timer's initial value
	__raw_writel(value, MISC_IO_ADDR(TCTVR_OFFSET+(id<<2)));

	return 0;
}

static int timer0_set_next(unsigned long cycles,
				struct clock_event_device *evt)
{
	return rtk_clkevt_set_next(TIMER0, cycles, evt);
}
static void timer0_set_mode(enum clock_event_mode mode,
				struct clock_event_device *evt)
{
	rtk_clkevt_set_mode(TIMER0, mode, evt);
}

static struct clocksource rtk_cs = {
	.name	= "rtk_counter",
	.rating	= 400,
	.read	= rtk_read_sched_clock,
	.mask	= CLOCKSOURCE_MASK(32),
	.flags	= CLOCK_SOURCE_IS_CONTINUOUS,
	.mult	= 0,
	.shift	= 10,
};

static struct clock_event_device timer0_clockevent = {
	.rating			= 100,
	.shift			= 32,
	.features		= CLOCK_EVT_FEAT_PERIODIC | CLOCK_EVT_FEAT_ONESHOT,
	.set_next_event = timer0_set_next,
	.set_mode		= timer0_set_mode,
};

static struct irqaction timer0_irq = {
	.flags			= IRQF_TIMER | IRQF_IRQPOLL | IRQF_DISABLED,
	.handler		= rtk119x_clock_event_isr,
	.dev_id			= &timer0_clockevent,
};

#ifdef CONFIG_HIGH_RES_TIMERS
static int timer1_set_next(unsigned long cycles,
				struct clock_event_device *evt)
{
	return rtk_clkevt_set_next(TIMER1, cycles, evt);
}
static void timer1_set_mode(enum clock_event_mode mode,
				struct clock_event_device *evt)
{
	rtk_clkevt_set_mode(TIMER1, mode, evt);
}
static struct clock_event_device timer1_clockevent = {
	.rating			= 400,
	.shift			= 32,
	.features		= CLOCK_EVT_FEAT_PERIODIC | CLOCK_EVT_FEAT_ONESHOT,
	.set_next_event = timer1_set_next,
	.set_mode		= timer1_set_mode,
};

static struct irqaction timer1_irq = {
	.flags			= IRQF_TIMER | IRQF_DISABLED,
	.handler		= rtk119x_clock_event_isr,
	.dev_id			= &timer1_clockevent,
};
#endif

struct rtk_clock_event_device {
	int		index;
	struct clock_event_device *evt;
	struct irqaction *irq_action;
};

static struct rtk_clock_event_device rtk_evt[] = {
	{ 0, &timer0_clockevent, &timer0_irq },
	{ 1, &timer1_clockevent, &timer1_irq },
};


static irqreturn_t rtk119x_clock_event_isr(int irq, void *dev_id)
{
	struct rtk_clock_event_device *clkevt = (struct rtk_clock_event_device*)dev_id;
	struct clock_event_device *evt = clkevt->evt;
	int nr = clkevt->index;

	rtd_setbits(MISC_IO_ADDR(ISR_OFFSET), UMSK_TC_shift[nr]);

	if (!evt->event_handler) {
		pr_info("Spurious rtk119x timer interrupt %d", irq);
		return IRQ_NONE;
	}

	evt->event_handler(evt);

	return IRQ_HANDLED;
}

static int rtk_clkevt_set_next(int nr, unsigned long cycles,
				struct clock_event_device *evt)
{
	unsigned int cnt;
	int res;

	timer_control(nr, HWT_INT_ENABLE);
	cnt = timer_get_value(nr);
	cnt += cycles;
	timer_set_target(nr, cnt);

	res = ((timer_get_value(nr) - cnt) > 0) ? -ETIME : 0;

	return res;
}

static void rtk_clkevt_set_mode(int nr, enum clock_event_mode mode,
				struct clock_event_device *evt)
{
	switch (mode) {
	case CLOCK_EVT_MODE_PERIODIC:
		create_timer(nr, DIV_ROUND_UP(clk_freq, HZ), TIMER);
		break;
	case CLOCK_EVT_MODE_ONESHOT:
		/* period set, and timer enabled in 'next event' hook */
		create_timer(nr, DIV_ROUND_UP(clk_freq, HZ), COUNTER);
		break;
	case CLOCK_EVT_MODE_SHUTDOWN:
        sTimerSuspendData[nr].value = timer_get_value(nr);
        sTimerSuspendData[nr].mode = timer_get_mode(nr);
		timer_control(nr, HWT_INT_DISABLE);
#if 0
		timer_control(nr, HWT_STOP);
#else
		timer_control(nr, HWT_PAUSE);
#endif
		break;
	case CLOCK_EVT_MODE_RESUME:
		printk("ready to resume clock event device...timer%d\n",nr);
        timer_set_value(nr, sTimerSuspendData[nr].value);
#if 0
		timer_control(nr, HWT_INT_CLEAR);
		timer_control(nr, HWT_RESUME);
#else
		create_timer(nr, DIV_ROUND_UP(clk_freq, HZ), sTimerSuspendData[nr].mode);
#endif
		break;
	case CLOCK_EVT_MODE_UNUSED:
		break;
	}
}

void rtk_clockevent_init(int index, const char *name,
		void __iomem *base, int irq, unsigned long freq)
{
	struct rtk_clock_event_device *clkevt = &rtk_evt[index];
	struct clock_event_device *evt = clkevt->evt;

	timer_base			= base;
	clk_freq			= freq;

	evt->irq			= irq;
	evt->name			= name;

	clockevents_calc_mult_shift(evt, freq, 5);

	evt->max_delta_ns	= clockevent_delta2ns(0x7fffffff, evt);
	evt->min_delta_ns	= clockevent_delta2ns(5000, evt);
//	evt->cpumask		= cpumask_of(cpu);
	evt->cpumask		= cpu_all_mask;

    memset(sTimerSuspendData,0,sizeof(sTimerSuspendData[0]) * TIMER_MAX);

	clkevt->irq_action->name = name;
	clkevt->irq_action->irq = irq;
	clkevt->irq_action->dev_id = clkevt;

	setup_irq(irq, clkevt->irq_action);

	create_timer(index, DIV_ROUND_UP(clk_freq, HZ), TIMER);

	clockevents_register_device(evt);
}

int timer_set_mode(unsigned char id, unsigned char mode)
{
	switch(mode) {
		case COUNTER:
			rtd_clearbits((MISC_IO_ADDR(TCCR_OFFSET + (id<<2))), TIMERINFO_TIMER_MODE);
			break;
		case TIMER:
			rtd_setbits((MISC_IO_ADDR(TCCR_OFFSET + (id<<2))), TIMERINFO_TIMER_MODE);
			break;
		default:
			return 1;
	}

	return 0;
}

unsigned char timer_get_mode(unsigned char id)
{
	unsigned int reg =  __raw_readl(MISC_IO_ADDR(TCCR_OFFSET + (id<<2)));
    return (reg & TIMERINFO_TIMER_MODE)? TIMER : COUNTER;
}

int timer_control(unsigned char id, unsigned int cmd)
{
	switch (cmd) {
		case HWT_INT_CLEAR:
			rtd_setbits(MISC_IO_ADDR(UMSK_ISR_OFFSET), UMSK_TC_shift[id]);
			break;
		case HWT_START:
			rtd_setbits(MISC_IO_ADDR(TCCR_OFFSET+(id<<2)), TIMERINFO_TIMER_ENABLE);
			// Clear Interrupt Pending (must after enable)
			rtd_setbits(MISC_IO_ADDR(ISR_OFFSET), UMSK_TC_shift[id]);
			break;
		case HWT_STOP:
			rtd_clearbits(MISC_IO_ADDR(TCCR_OFFSET+(id<<2)), TIMERINFO_TIMER_ENABLE);
			break;
		case HWT_PAUSE:
			rtd_setbits(MISC_IO_ADDR(TCCR_OFFSET+(id<<2)), TIMERINFO_TIMER_PAUSE);
			break;
		case HWT_RESUME:
			rtd_clearbits(MISC_IO_ADDR(TCCR_OFFSET+(id<<2)), TIMERINFO_TIMER_PAUSE);
			break;
		case HWT_INT_ENABLE:
			rtd_setbits(MISC_IO_ADDR(TCICR_OFFSET+(id<<2)), TIMERINFO_INTERRUPT_ENABLE);
			break;
		case HWT_INT_DISABLE:
			rtd_clearbits(MISC_IO_ADDR(TCICR_OFFSET+(id<<2)), TIMERINFO_INTERRUPT_ENABLE);
			break;
		default:
			return 1;
	}
	return 0;
}

int create_timer(unsigned char id, unsigned int interval, unsigned char mode)
{
	//Disable Interrupt
	timer_control(id, HWT_INT_DISABLE);

	//Disable Timer
	timer_control(id, HWT_STOP);

	//Set The Initial Value
	//timer_set_value(id, 0);
	timer_set_value(id, sTimerSuspendData[id].value);

	//Set The Target Value
	timer_set_target(id, interval);

	//Enable Timer Mode
	timer_set_mode(id, mode);

	//Enable Timer
	timer_control(id, HWT_START);

	//Enable Interrupt
	timer_control(id, HWT_INT_ENABLE);

	return 0;
}
#if 0
void cevt_suspend(struct clock_event_device *evt)
{
}

void cevt_resume(struct clock_event_device *evt)
{
	timer_control(SYSTEM_TIMER, HWT_INT_DISABLE);
	timer_control(SYSTEM_TIMER, HWT_STOP);
}
#endif

/*
 * Override the global weak sched_clock symbol with this
 * local implementation which uses the clocksource to get some
 * better resolution when scheduling the kernel. We accept that
 * this wraps around for now, since it is just a relative time
 * stamp.
 */
//static DEFINE_SPINLOCK(clksrc_lock);

static cycle_t rtk_read_sched_clock(struct clocksource *cs)
{
#if 0
	unsigned long flags;
	u32 lower, upper;

	spin_lock_irqsave(&clksrc_lock, flags);

	lower = readl(IOMEM(0xfe01b578));
	upper = readl(IOMEM(0xfe01b57c)) & 0xffff;

	spin_unlock_irqrestore(&clksrc_lock, flags);
	return (((cycle_t)upper)<<32) | lower;
#else
	return (cycle_t)timer_get_value(TIMER2);
#endif
}

static void _rtk_clocksource_init(int nr)
{
	//Disable Timer
	timer_control(nr, HWT_STOP);

	//Set The Initial Value
	timer_set_value(nr, sTimerSuspendData[nr].value);
	//timer_set_value(nr, 0);

	//Set The Target Value
	timer_set_target(nr, DIV_ROUND_UP(clk_freq, HZ));

	//Enable Timer Mode
	timer_set_mode(nr, COUNTER);

	//Enable Timer
	timer_control(nr, HWT_START);

	timer_get_value(nr);
}

//void rtk_clocksource_init(unsigned rating, const char *name, unsigned long freq)
void rtk_clocksource_init(void)
{
	struct clocksource *cs = &rtk_cs;
#if 1
    _rtk_clocksource_init(TIMER2);
#else
#if 1
	//Disable Timer
	timer_control(TIMER2, HWT_STOP);

	//Set The Initial Value
	timer_set_value(TIMER2, 0);

	//Set The Target Value
	timer_set_target(TIMER2, DIV_ROUND_UP(clk_freq, HZ));

	//Enable Timer Mode
	timer_set_mode(TIMER2, COUNTER);

	//Enable Timer
	timer_control(TIMER2, HWT_START);
	timer_get_value(TIMER2);
#else
	// clear counter
	writel(0x0, IOMEM(0xfe01b570));
	// enable counter
	writel(0x01, IOMEM(0xfe01b570));
#endif
#endif

	if (clocksource_register_hz(cs, clk_freq))
		panic("rtk119x_clocksource_timer: can't register clocksource\n");
}

int rtk_clocksource_suspend(void)
{
    int nr = TIMER2;

#if 1
    for (nr=0; nr<TIMER_MAX; nr++)
#endif
        sTimerSuspendData[nr].value = timer_get_value(nr);

    return 0;
}

void rtk_clocksource_resume(void)
{
    _rtk_clocksource_init(TIMER2);
}
