<script setup>
import { ref, computed } from 'vue'
import { useAnimatedCounter } from '@/services/utils'
import TooltipComp from './TooltipComp.vue'


const props = defineProps({
  title: {
    type: String,
    required: true
  },
  val: {
    type: Number,
    required: true
  },
  type: {
    type: String,
    default: ''
  },
  decimals: {
    type: Number,
    default: 0
  },
  tooltip: {
    type: String,
    default: ''
  }
})

const displayVal = useAnimatedCounter(props.val)

const text = computed(() => {
  let str = ''
  if(props.type.includes("beneficial") && displayVal.value >= 0){
    str += '+'
  }
  if(props.type.includes("percentile")){
    str += (displayVal.value * 100).toFixed(props.decimals) + '%'
  }
  else if(props.type.includes("currency")){
    str += '$' + displayVal.value.toLocaleString(undefined, { minimumFractionDigits: props.decimals, maximumFractionDigits: props.decimals })

  }else{
    str += displayVal.value.toFixed(props.decimals)
  }
  return str
})

const text_color = computed(() => {
  if(props.type.includes("beneficial")){
    return displayVal.value >= 0 ? 'text-positive' : 'text-negative'
  }else{
    return ''
  }
})

</script>

<template>
    <div class="relative bg-bg-card border border-border rounded-xl p-6 transition hover:bg-bg-card-hover">
          <div class="flex items-center gap-1.5 mb-2">
            <span class="text-xs font-semibold uppercase tracking-wider text-text-secondary">{{ props.title }}</span>
            <TooltipComp v-if="props.tooltip" :content="props.tooltip" />
          </div>
          <div :class="['text-2xl font-bold', text_color]">{{text}}</div>
    </div>
</template>

<style scoped>
.tooltip-fade-enter-active,
.tooltip-fade-leave-active {
  transition: opacity 0.15s ease;
}
.tooltip-fade-enter-from,
.tooltip-fade-leave-to {
  opacity: 0;
}
</style>