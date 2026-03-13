<script setup>
import { ref, computed } from 'vue'


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

const showTooltip = ref(false)

const text = computed(() => {
  let str = ''
  if(props.type.includes("beneficial") && props.vals >= 0){
    str += '+'
  }
  if(props.type.includes("percentile")){
    str += (props.val * 100).toFixed(props.decimals) + '%'
  }
  else if(props.type.includes("currency")){
    str += '$' + props.val.toLocaleString(undefined, { minimumFractionDigits: props.decimals, maximumFractionDigits: props.decimals })

  }else{
    str += props.val.toFixed(props.decimals)
  }
  return str
})

const text_color = computed(() => {
  if(props.type.includes("beneficial")){
    return props.val >= 0 ? 'text-positive' : 'text-negative'
  }else{
    return ''
  }
})

</script>

<template>
    <div class="relative bg-bg-card border border-border rounded-xl p-6 transition hover:bg-bg-card-hover"
         @mouseenter="showTooltip = true" @mouseleave="showTooltip = false">
          <div class="text-xs font-semibold uppercase tracking-wider text-text-secondary mb-2">{{ props.title }}</div>
          <div :class="['text-2xl font-bold ', text_color]">{{text}}</div>

          <Transition name="tooltip-fade">
            <div v-if="showTooltip && tooltip"
                 class="absolute z-10 left-1/2 -translate-x-1/2 bottom-full mb-2 px-3 py-2 text-xs text-text-primary bg-bg-secondary border border-border rounded-lg shadow-lg max-w-56 text-center whitespace-normal pointer-events-none">
              {{ tooltip }}
            </div>
          </Transition>
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