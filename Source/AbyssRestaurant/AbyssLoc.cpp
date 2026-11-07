#include "AbyssLoc.h"

namespace
{
	bool GChinese = true;

	struct FPair { const TCHAR* En; const TCHAR* Zh; };

	// Whole-string translations (exact match)
	const FPair GExact[] = {
		{ TEXT("Yellowtail"), TEXT("黄尾鲷") },
		{ TEXT("Tuna"), TEXT("金枪鱼") },
		{ TEXT("Grouper"), TEXT("石斑鱼") },
		{ TEXT("SONAR"), TEXT("声呐") },
		{ TEXT("DEPTH"), TEXT("深度") },
		{ TEXT("BATTERY"), TEXT("电量") },
		{ TEXT("FRIDGE"), TEXT("冰箱") },
		{ TEXT("Satisfaction"), TEXT("满意度") },
		{ TEXT("GOOD"), TEXT("良好") },
		{ TEXT("Yum"), TEXT("好吃") },
		{ TEXT("Hmm..."), TEXT("嗯……") },
		{ TEXT("MAX"), TEXT("已满级") },
		{ TEXT("COOKING"), TEXT("烹饪中") },
		{ TEXT("NO STOCK"), TEXT("缺货") },
		{ TEXT("O2"), TEXT("氧气") },
		{ TEXT("N"), TEXT("北") }, { TEXT("NE"), TEXT("东北") }, { TEXT("E"), TEXT("东") }, { TEXT("SE"), TEXT("东南") },
		{ TEXT("S"), TEXT("南") }, { TEXT("SW"), TEXT("西南") }, { TEXT("W"), TEXT("西") }, { TEXT("NW"), TEXT("西北") },
		{ TEXT("PERFECT!"), TEXT("完美！") },
		{ TEXT("Perfect!"), TEXT("完美！") },
		{ TEXT("BURNT..."), TEXT("烧焦了……") },
		{ TEXT("UNDERCOOKED..."), TEXT("没熟……") },
		{ TEXT("Delicious!"), TEXT("太好吃了！") },
		{ TEXT("too strong"), TEXT("太强壮") },
		{ TEXT("CLAW..."), TEXT("抓取中……") },
		{ TEXT("Net"), TEXT("渔网") },
		{ TEXT("Rice"), TEXT("米饭") },
		{ TEXT("Fish Slice"), TEXT("鱼片") },
		{ TEXT("Fish Steak"), TEXT("鱼排") },
		{ TEXT("Lemon"), TEXT("柠檬") },
		{ TEXT("Greens"), TEXT("蔬菜") },
		{ TEXT("Seaweed"), TEXT("海藻") },
		{ TEXT("Wasabi"), TEXT("芥末") },
		{ TEXT("Sesame"), TEXT("芝麻") },
		{ TEXT("1 Slice"), TEXT("1 切片") },
		{ TEXT("2 Cook"), TEXT("2 火候") },
		{ TEXT("3 Plate"), TEXT("3 装盘") },
		{ TEXT("FLIP"), TEXT("翻面") },
		{ TEXT("DONE"), TEXT("起锅") },
		{ TEXT("ORDER"), TEXT("订单") },
		{ TEXT("SERVE"), TEXT("出餐") },
		{ TEXT("Clean cut!"), TEXT("切得漂亮！") },
		{ TEXT("Good cut"), TEXT("切得不错") },
		{ TEXT("Messy cut..."), TEXT("切歪了……") },
		{ TEXT("Perfect flip!"), TEXT("完美翻面！") },
		{ TEXT("Flipped"), TEXT("已翻面") },
		{ TEXT("Bad timing..."), TEXT("时机不对……") },
		{ TEXT("Perfectly cooked!"), TEXT("火候完美！") },
		{ TEXT("Nicely done"), TEXT("火候不错") },
		{ TEXT("Overcooked..."), TEXT("煎过头了……") },
		{ TEXT("Forgot to flip - one side burnt!"), TEXT("忘记翻面 - 一面焦了！") },
		{ TEXT("Flip it in the first green zone!"), TEXT("指针到第一个绿区时翻面！") },
		{ TEXT("Take it off in the second green zone!"), TEXT("指针到第二个绿区时起锅！") },
		{ TEXT("Drag ingredients onto the plate"), TEXT("把配料拖到盘子上") },
		{ TEXT("[Click] / [Space] / [E]"), TEXT("[左键] / [空格] / [E]") },
		{ TEXT("SONAR CONTACT: UNKNOWN"), TEXT("声呐接触：未知目标") },
		{ TEXT("Massive signal  -  312 m  -  rising"), TEXT("巨型回波  -  312 米  -  正在上浮") },
		{ TEXT("ABYSS DINING"), TEXT("深海食堂  ABYSS DINING") },
		{ TEXT("End of demo  -  thanks for playing"), TEXT("Demo 结束  -  感谢游玩") },
		{ TEXT("[ENTER]  Keep running the restaurant"), TEXT("[回车]  继续经营餐厅") },
		{ TEXT("Finish the current dish first"), TEXT("先做完这道菜") },
		{ TEXT("Serve the dish in your hands first"), TEXT("先把手上的菜端给客人") },
		{ TEXT("Carry the dish to the guest - follow the green marker"), TEXT("把菜端给客人 - 跟着绿色标记走") },
		{ TEXT("[E] Take order"), TEXT("[E] 点单") },
		{ TEXT("Shop"), TEXT("采购") }, { TEXT("Restaurant expansion"), TEXT("餐厅扩建") }, { TEXT("Battery capacity"), TEXT("电池容量") }, { TEXT("Searchlight & claw range"), TEXT("探照灯与机械爪") }, { TEXT("Diver oxygen tank"), TEXT("潜水氧气瓶") }, { TEXT("Upgrade"), TEXT("升级") },
		{ TEXT("Restock by catching fish at sea after closing"), TEXT("打烊后出海捕鱼来补货") },
		{ TEXT("Decor shop coming soon"), TEXT("装饰商店即将开放") },
		{ TEXT("Back"), TEXT("返回") },
		{ TEXT("Menu"), TEXT("菜单") }, { TEXT("Decor"), TEXT("装饰") }, { TEXT("Tasks"), TEXT("任务") }, { TEXT("Codex"), TEXT("图鉴") },
		{ TEXT("< Dining room [E]"), TEXT("< 回到餐厅 [E]") },
		{ TEXT("Go fishing >"), TEXT("出海捕鱼 >") },
		{ TEXT("OPEN"), TEXT("营业中") }, { TEXT("CLOSED"), TEXT("打烊") },
		{ TEXT("Serve"), TEXT("出餐") }, { TEXT("DING!"), TEXT("叮！") }, { TEXT("Plate"), TEXT("盘子") },
		{ TEXT("Drop a fillet here"), TEXT("把鱼柳拖到砧板上") },
		{ TEXT("BURNT!"), TEXT("烧焦了！") }, { TEXT("BURNING!"), TEXT("快糊了！") },
		{ TEXT("Click to FLIP"), TEXT("点击翻面") }, { TEXT("READY - drag to plate"), TEXT("熟了 - 拖到盘子") },
		{ TEXT("Yellowtail fillet"), TEXT("黄尾鲷鱼柳") }, { TEXT("Tuna fillet"), TEXT("金枪鱼鱼柳") }, { TEXT("Grouper fillet"), TEXT("石斑鱼鱼柳") },
		{ TEXT("Raw steak"), TEXT("生鱼排") }, { TEXT("Seared steak"), TEXT("煎鱼排") }, { TEXT("Burnt steak"), TEXT("焦鱼排") },
		{ TEXT("Something is burning on the stove!"), TEXT("炉子上有东西烧糊了！") },
		{ TEXT("Finish tonight's service before heading out"), TEXT("营业结束后才能出海") },
		{ TEXT("Out of stock - catch more at sea"), TEXT("没货了 - 出海多抓点") },
		{ TEXT("Cut it on the board first!"), TEXT("先在砧板上切好！") },
		{ TEXT("Cook it in a pan first!"), TEXT("先放进锅里煎！") },
		{ TEXT("The plate is full"), TEXT("盘子满了") }, { TEXT("The board is busy"), TEXT("砧板上还有东西") },
		{ TEXT("Click on the gold lines to cut"), TEXT("点击金色虚线下刀") },
		{ TEXT("Nice flip!"), TEXT("漂亮的翻面！") }, { TEXT("Flipped a bit early"), TEXT("翻早了") }, { TEXT("Flipped a bit late"), TEXT("翻晚了") },
		{ TEXT("Thrown away"), TEXT("扔掉了") }, { TEXT("No order selected"), TEXT("没有选中订单") }, { TEXT("The plate is empty"), TEXT("盘子是空的") },
		{ TEXT("No orders yet - take orders at the tables [E]"), TEXT("还没有订单 - 去桌边给客人点单 [E]") },
		{ TEXT("Drag ingredients   |   click the gold lines to cut   |   click a pan to flip   |   ring the bell to serve"), TEXT("拖动食材   |   点击金线下刀   |   点击锅子翻面   |   按铃出餐") },
		{ TEXT("TONIGHT"), TEXT("今晚任务") }, { TEXT("FISH CODEX"), TEXT("鱼类图鉴") }, { TEXT("click anywhere to close"), TEXT("点击任意处关闭") },
		{ TEXT("[H] Recipe book"), TEXT("[H] 菜谱") },
		{ TEXT("RECIPE BOOK"), TEXT("菜谱") },
		{ TEXT("[H] close"), TEXT("[H] 关闭") },
		{ TEXT("Take orders at the tables [E]  ->  cook at the stations  ->  carry the plate back to the guest [E]"), TEXT("到桌边点单 [E]  ->  去各工位做菜  ->  把菜端回给客人 [E]") },
	};

	// Phrase replacements (applied longest first)
	const FPair GPhrases[] = {
		{ TEXT("Yellowtail Nigiri"), TEXT("黄尾鲷握寿司") },
		{ TEXT("Tuna Sashimi Set"), TEXT("金枪鱼刺身套餐") },
		{ TEXT("Tuna Teppanyaki"), TEXT("金枪鱼铁板烧") },
		{ TEXT("Cut on the gold lines  "), TEXT("沿着金色虚线切  ") },
		{ TEXT("PERFECT!  "), TEXT("完美！  ") },
		{ TEXT("GOOD  "), TEXT("良好  ") },
		{ TEXT("MISS  "), TEXT("失误  ") },
		{ TEXT("Prep "), TEXT("备料 ") },
		{ TEXT("Cook "), TEXT("火候 ") },
		{ TEXT("Grouper Teppanyaki"), TEXT("石斑鱼铁板烧") },
		{ TEXT("Kelp Salad"), TEXT("海藻沙拉") },
		{ TEXT("Bluefin Tuna"), TEXT("蓝鳍金枪鱼") },
		{ TEXT("Rare Grouper"), TEXT("稀有石斑鱼") },
		{ TEXT("Yellowtail"), TEXT("黄尾鲷") },
		{ TEXT("Grouper"), TEXT("石斑鱼") },
		{ TEXT("Sub Claw"), TEXT("潜艇机械爪") },
		{ TEXT("Speargun"), TEXT("鱼叉枪") },
		{ TEXT("Hand Net"), TEXT("手持渔网") },

		{ TEXT("Day 1 - Find fish. Hover & press [E] to dive out with net/speargun. Dock with [E]."), TEXT("第 1 天 - 去找鱼吧。悬停时按 [E] 出舱用渔网/鱼叉捕鱼，在泊位按 [E] 停靠。") },
		{ TEXT("Already at max level"), TEXT("已经满级了") },
		{ TEXT("Not enough money"), TEXT("钱不够") },
		{ TEXT("Battery upgraded - longer dives!"), TEXT("电池已升级 - 可以潜得更久！") },
		{ TEXT("Searchlight upgraded - brighter beam, longer claw range!"), TEXT("探照灯已升级 - 更亮、机械爪距离更远！") },
		{ TEXT("Oxygen tank upgraded - the diver stays out longer!"), TEXT("氧气瓶已升级 - 潜水员可以待得更久！") },
		{ TEXT("Diving! [1] Speargun  [2] Net  [LMB] Use  -  Return to the hatch with [E]"), TEXT("出舱！[1] 鱼叉枪  [2] 渔网  [左键] 使用 - 回到舱门按 [E]") },
		{ TEXT("Out of oxygen! Emergency recall to the sub - lost "), TEXT("氧气耗尽！紧急召回潜艇 - 丢失 ") },
		{ TEXT("Back aboard the submarine"), TEXT("已回到潜艇") },
		{ TEXT("Battery depleted! Emergency tow back to base... The restaurant opens."), TEXT("电量耗尽！紧急拖回基地……餐厅开始营业。") },
		{ TEXT("The Abyss Restaurant is open! Press 1-4 to cook an order, SPACE to plate."), TEXT("深海食堂开张啦！按 1-4 做菜，按空格出菜。") },
		{ TEXT(" - Tuna roam further out. Rare groupers lurk deep in the east trench."), TEXT(" - 金枪鱼在外围出没，稀有石斑鱼藏在东边海沟深处。") },

		{ TEXT("Cargo hold full - return to the sub [E]"), TEXT("货舱满了 - 回潜艇 [E]") },
		{ TEXT("Cargo hold full - return to the dock [E]"), TEXT("货舱满了 - 回泊位 [E]") },
		{ TEXT("The spear glances off the "), TEXT("鱼叉被弹开了：") },
		{ TEXT("Speared: "), TEXT("鱼叉命中：") },
		{ TEXT("Hit! The "), TEXT("命中！") },
		{ TEXT(" is stunned - shoot again!"), TEXT(" 被打晕了 - 再补一发！") },
		{ TEXT("Netted "), TEXT("网住了 ") },
		{ TEXT(" fish!"), TEXT(" 条鱼！") },
		{ TEXT(" fish"), TEXT(" 条鱼") },
		{ TEXT("Too strong for the net - it broke free! Use the speargun [1]"), TEXT("太强壮了，挣脱了渔网！请用鱼叉枪 [1]") },
		{ TEXT("Slow down / hover to dive out"), TEXT("减速/悬停后才能出舱") },
		{ TEXT(": too small for the claw - dive out [E] and use the NET"), TEXT("：太小，机械爪抓不住 - 按 [E] 出舱用渔网") },
		{ TEXT("Caught: "), TEXT("捕获：") },
		{ TEXT("(+1 "), TEXT("（+1 ") },

		{ TEXT("Finish the current dish first - press SPACE"), TEXT("先做完这道菜 - 按空格") },
		{ TEXT("! Catch more tomorrow."), TEXT("！明天多抓一些。") },
		{ TEXT("Out of "), TEXT("没有") },
		{ TEXT("A customer got tired of waiting and left!"), TEXT("有客人等不及走掉了！") },

		{ TEXT("Claw        [Hold LMB]"), TEXT("机械爪      [按住左键]") },
		{ TEXT("Dive out   [E] (hover)"), TEXT("出舱       [E]（悬停）") },
		{ TEXT("Light            [F]"), TEXT("探照灯          [F]") },
		{ TEXT("Boost        [Shift]"), TEXT("加速         [Shift]") },
		{ TEXT("Up / Down  [Space/Ctrl]"), TEXT("上浮/下潜 [空格/Ctrl]") },
		{ TEXT("Language     [Tab]"), TEXT("语言 中/EN   [Tab]") },
		{ TEXT("HULL PRESSURE HIGH"), TEXT("船体压力过高") },
		{ TEXT("LOW BATTERY - RETURN TO DOCK"), TEXT("电量不足 - 请返回泊位") },
		{ TEXT("[E]  Dock & open the restaurant"), TEXT("[E]  停靠并开始营业") },
		{ TEXT("[E] Dive out"), TEXT("[E] 出舱潜水") },
		{ TEXT("  -  Reef Survey"), TEXT("  -  珊瑚礁勘察") },
		{ TEXT("Catch fish for tonight  "), TEXT("为今晚备货  ") },
		{ TEXT("Find a Rare Grouper near the trench  "), TEXT("在海沟附近找到稀有石斑鱼  ") },
		{ TEXT("Dive out [E] and use the net on yellowtail"), TEXT("按 [E] 出舱，用渔网捕黄尾鲷") },
		{ TEXT("Return to the docking bay  [E]"), TEXT("返回泊位  [E]") },
		{ TEXT("DOCK "), TEXT("泊位 ") },
		{ TEXT("SUB "), TEXT("潜艇 ") },
		{ TEXT("CARGO  "), TEXT("货舱  ") },
		{ TEXT("[Hold LMB] Claw"), TEXT("[按住左键] 机械爪") },
		{ TEXT("Dive out [E] to catch"), TEXT("按 [E] 出舱捕捉") },
		{ TEXT("HIT "), TEXT("命中 ") },
		{ TEXT("limit "), TEXT("极限 ") },
		{ TEXT("LOW OXYGEN - RETURN TO THE SUB"), TEXT("氧气不足 - 请返回潜艇") },
		{ TEXT("precise - big fish"), TEXT("精准 - 适合大鱼") },
		{ TEXT("close - catches 3"), TEXT("近距离 - 一次 3 条") },
		{ TEXT("[LMB] Swing net"), TEXT("[左键] 挥网") },
		{ TEXT("TOO FAR FROM THE SUB  "), TEXT("离潜艇太远  ") },
		{ TEXT("DIVER DEPTH LIMIT - use the sub claw for deep fish"), TEXT("已达潜水员深度极限 - 深处的鱼请用潜艇机械爪") },
		{ TEXT("[E]  Board the submarine"), TEXT("[E]  返回潜艇") },
		{ TEXT("The Abyss Restaurant"), TEXT("深海食堂") },
		{ TEXT("Guests tonight: "), TEXT("今晚剩余客人：") },
		{ TEXT(" left"), TEXT(" 位") },
		{ TEXT("Orders "), TEXT("订单 ") },
		{ TEXT("Waiting for guests..."), TEXT("等待客人……") },
		{ TEXT("All guests served"), TEXT("客人都招待完了") },
		{ TEXT("Press 1 - 4 to start cooking an order"), TEXT("按 1 - 4 开始做菜") },
		{ TEXT(" Complete"), TEXT(" 营业结束") },
		{ TEXT("Night "), TEXT("第 ") },
		{ TEXT("Guests who left angry"), TEXT("生气离开的客人") },
		{ TEXT("Guests served"), TEXT("已招待客人") },
		{ TEXT("Perfect dishes"), TEXT("完美菜品") },
		{ TEXT("Earned tonight"), TEXT("今晚收入") },
		{ TEXT("UPGRADES   (money: "), TEXT("升级   （资金：") },
		{ TEXT("Battery capacity"), TEXT("电池容量") },
		{ TEXT("Searchlight & claw range"), TEXT("探照灯与机械爪距离") },
		{ TEXT("Diver oxygen tank"), TEXT("潜水员氧气瓶") },
		{ TEXT("[ENTER]  Dive again"), TEXT("[回车]  再次出海") },
		{ TEXT("Walk to the kitchen counter to cook  (WASD)"), TEXT("走到厨房柜台才能做菜（WASD 移动）") },
		{ TEXT("Walk to the kitchen counter first"), TEXT("先走到厨房柜台") },
		{ TEXT("Welcome in! Walk around freely (WASD). Cook at the kitchen counter."), TEXT("欢迎回店！可以自由走动（WASD），到厨房柜台做菜。") },
		{ TEXT("[E] Cook the next order   (or 1-4 to pick one)"), TEXT("[E] 做下一道菜（或按 1-4 选择订单）") },
		{ TEXT("[SPACE] / [E]"), TEXT("[空格] / [E] 出菜") },
		{ TEXT("KITCHEN"), TEXT("厨房") },
		{ TEXT("Restaurant expanded - more tables and more guests tonight!"), TEXT("餐厅扩建完成 - 更多桌位，今晚客人更多！") },
		{ TEXT("Restaurant expansion  "), TEXT("餐厅扩建  ") },
		{ TEXT("Go to the "), TEXT("请前往 ") },
		{ TEXT("Order taken: "), TEXT("已接单：") },
		{ TEXT("Guests waiting to order: "), TEXT("等待点单的客人：") }, { TEXT("Orders to cook: "), TEXT("待做订单：") },
		{ TEXT("Served tonight: "), TEXT("今晚已上菜：") }, { TEXT("(perfect "), TEXT("（完美 ") },
		{ TEXT("Guests still coming: "), TEXT("还会来的客人：") }, { TEXT("Earned tonight: "), TEXT("今晚收入：") },
		{ TEXT("in stock: "), TEXT("库存：") }, { TEXT("Day "), TEXT("第 ") },
		{ TEXT("Serve: "), TEXT("上菜：") },
		{ TEXT("Carrying "), TEXT("端着 ") },
		{ TEXT(" - bring it to the guest and press [E]"), TEXT(" - 走到客人旁按 [E] 上菜") },
		{ TEXT("(tip +"), TEXT("(小费 +") },
		{ TEXT("Needs: "), TEXT("需要：") },
		{ TEXT("PREP TABLE"), TEXT("备料台") },
		{ TEXT("SERVING COUNTER"), TEXT("出餐台") },
		{ TEXT("STOVE"), TEXT("炉灶") },
		{ TEXT("Day "), TEXT("第 ") },
	};

	TMap<FString, FString>& Cache()
	{
		static TMap<FString, FString> C;
		return C;
	}

	TArray<const FPair*>& SortedPhrases()
	{
		static TArray<const FPair*> P;
		if (P.Num() == 0)
		{
			for (const FPair& X : GPhrases) P.Add(&X);
			P.Sort([](const FPair& A, const FPair& B) { return FCString::Strlen(A.En) > FCString::Strlen(B.En); });
		}
		return P;
	}
}

namespace AbyssLoc
{
	bool IsChinese() { return GChinese; }
	void SetChinese(bool b) { GChinese = b; }
	void Toggle() { GChinese = !GChinese; }

	const FString& Translate(const FString& En)
	{
		if (!GChinese) return En;
		TMap<FString, FString>& C = Cache();
		if (const FString* Hit = C.Find(En)) return *Hit;
		if (C.Num() > 4000) C.Reset(); // dynamic strings (timers etc) - keep the cache bounded

		for (const FPair& X : GExact)
		{
			if (En.Equals(X.En, ESearchCase::CaseSensitive))
			{
				return C.Add(En, X.Zh);
			}
		}
		FString Out = En;
		for (const FPair* X : SortedPhrases())
		{
			Out.ReplaceInline(X->En, X->Zh, ESearchCase::CaseSensitive);
		}
		// "Day 3" -> "第 3 天", "Night 3 营业结束" -> "第 3 晚营业结束"
		if (Out.StartsWith(TEXT("第 ")))
		{
			int32 i = 2;
			while (i < Out.Len() && FChar::IsDigit(Out[i])) ++i;
			const bool bNight = En.StartsWith(TEXT("Night "));
			const FString Unit = bNight ? TEXT(" 晚") : TEXT(" 天");
			if (!Out.Mid(i, Unit.Len()).Equals(Unit)) Out.InsertAt(i, *Unit);
		}
		return C.Add(En, Out);
	}
}
